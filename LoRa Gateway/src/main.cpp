#include <Arduino.h>
// #include <PubSubClient.h>
// #include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include <AntaresESPMQTT.h>
#include "time.h"

// START DISPLAY PREQUISITE
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Ganti alamat I2C sesuai dengan hasil scan
#define OLED_ADDR 0x3C

// Inisialisasi LCD dan OLED
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define ACCESSKEY "YOUR-ACCESS-KEY"       // Antares account access key
#define WIFISSID "YOUR-WIFI-SSID"         // Wi-Fi SSID to connect to
#define PASSWORD "YOUR-WIFI-PASSWORD"     // Wi-Fi password

#define projectName "YOUR-APPLICATION-NAME"   // Name of the application created in Antares
#define deviceName "YOUR-DEVICE-NAME"     // Name of the device created in Antares

AntaresESPMQTT antares(ACCESSKEY);

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

const char* ssid = "crustea";
const char* password = "crustea1234";
const char* ntpServer = "pool.ntp.org"; // Server NTP
const long  gmtOffset_sec = 7 * 3600;   // GMT+7 untuk WIB
const int   daylightOffset_sec = 0;     // Tidak ada daylight saving di Indonesia
char status_koneksi[8] = "Offline";
String LoRaData;
struct tm timeinfo;

// WiFiClient wifiClient;
// PubSubClient client(wifiClient);

void Display();
void LoRaSetUp();
void SetupWifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
void SendMessageToServer();
void LoRaReceiver();
void updateTime();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  LoRaSetUp();
  pinMode(led, OUTPUT);

  // SetupWifi();
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

  antares.setDebug(true);
  antares.wifiConnection(WIFISSID, PASSWORD);
  antares.setMqttServer();
  
  /// Konfigurasi NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Menunggu sinkronisasi waktu...");

  // Ambil waktu pertama kali
  updateTime();
  delay(3500);
}

void loop() {
  // put your main code here, to run repeatedly:
  LoRaReceiver();
  antares.checkMqttConnection();
  // if (!client.connected()) {
  //   reconnect();
  // } else {
  //   strcpy(status_koneksi, "Online");
  // }

  if ( millis() - lastSend > 10000 ) { // Update and send only after 2 seconds
    SendMessageToServer();
    Serial.print("Send..");
    lastSend = millis();
  }
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
  Serial.println("unused");
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
  // Perbarui waktu sebelum mengirim data
  updateTime();

  // Format waktu menjadi string (YYYY-MM-DD HH:MM:SS)
  char timeStr[20];
  sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Tambahkan data sensor
  antares.add("timestamp", timeStr);  // ⏳ Menambahkan waktu ke payload
  antares.add("voltage", voltage);
  antares.add("current", current);
  antares.add("power", power);
  antares.add("energy", energy);
  antares.add("frequency", frequency);
  antares.add("power_factor", pf);

  // Kirim data ke Antares MQTT
  antares.publish(projectName, deviceName);
}

void updateTime() {
  if (getLocalTime(&timeinfo)) {
    Serial.println("Waktu diperbarui dari NTP!");
  } else {
    Serial.println("Gagal mendapatkan waktu dari NTP");
  }
}