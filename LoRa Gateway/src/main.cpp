#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>

// START DISPLAY PREQUISITE
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Ganti alamat I2C sesuai dengan hasil scan
#define OLED_ADDR 0x3C

// Inisialisasi LCD dan OLED
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define ss 5
#define rst 14
#define dio0 2

unsigned long lastSend;
unsigned long lastOn;
int led = 2;
int counter =0;

float voltage, current, power, energy, frequency, pf;

#define MSG_BUFFER_SIZE  (500)
char data[MSG_BUFFER_SIZE] = "{\"Voltage_PLN\":\"%.2f\",\"Current_PLN\":\"%.2f\",\"Power_PLN\":\"%.2f\",\"Energy_PLN\":\"%.2f\",\"Frekuensi_PLN\":\"%.2f\",\"Power_Faktor_PLN\":\"%.2f\"}";
char data_buffer[500];

#define TOKEN "ACCESS_TOKEN"

const char* ssid = "crustea";
const char* password = "crustea1234";
const char* mqtt_server = "18.140.254.213";
const int mqtt_port = 1883;
const char* topic = "v1/devices/me/telemetry";
char status_koneksi[8] = "Offline";
String LoRaData;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void Display();
void LoRaSetUp();
void SetupWifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
void SendMessageToServer();
void LoRaReceiver();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  LoRaSetUp();
  pinMode(led, OUTPUT);

  SetupWifi();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  oled.display();
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("CAPSTONE");
  oled.display();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  delay(3500);
}

void loop() {
  // put your main code here, to run repeatedly:
  LoRaReceiver();
  if (!client.connected()) {
    reconnect();
  } else {
    strcpy(status_koneksi, "Online");
  }

  if ( millis() - lastSend > 1000 ) { // Update and send only after 2 seconds
    SendMessageToServer();
    Serial.print("Send..");
    lastSend = millis();
  }

  client.loop();
}

void LoRaSetUp() {
  while (!Serial);
  Serial.println("LoRa Gateway Started!!");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF5);
  Serial.println("LoRa Initializing OK!");
}

void SetupWifi() {

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    strcpy(status_koneksi, "Offline");
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  strcpy(status_koneksi, "Online");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266 Device", TOKEN, NULL)) {
      Serial.println("connected");
      strcpy(status_koneksi, "Online");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void LoRaReceiver() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      Serial.print(LoRaData); 
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}

void SendMessageToServer() {
  client.publish(topic , LoRaData.c_str());
}
