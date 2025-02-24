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

#define ACCESSKEY "c1bf19abbffd6f70:2199a99279d7edd6"       // Antares account access key
#define WIFISSID "crustea"         // Wi-Fi SSID to connect to
#define PASSWORD "crustea1234"     // Wi-Fi password

#define projectName "AMI_Module"   // Name of the application created in Antares
#define deviceName "LoRa_Gateway"     // Name of the device created in Antares

AntaresESPMQTT antares(ACCESSKEY);

#define ss 5
#define rst 14
#define dio0 2

unsigned long lastSend;
unsigned long lastOn;
int led = 2;
int counter =0;

float voltage, current, power, energy, frequency, pf;

const int MSG_BUFFER_SIZE = 600;
char data[MSG_BUFFER_SIZE] = "{\"Voltage_PLN\":\"%.2f\",\"Current_PLN\":\"%.2f\",\"Power_PLN\":\"%.2f\",\"Energy_PLN\":\"%.2f\",\"Frekuensi_PLN\":\"%.2f\",\"Power_Faktor_PLN\":\"%.2f\"}";
char data_buffer[500];
char jsonData[MSG_BUFFER_SIZE];

const char* ssid = "crustea";
const char* password = "crustea1234";
const char* ntpServer = "pool.ntp.org"; // Server NTP
const long  gmtOffset_sec = 7 * 3600;   // GMT+7 untuk WIB
const int   daylightOffset_sec = 0;     // Tidak ada daylight saving di Indonesia
char status_koneksi[8] = "Offline";
String LoRaData;
struct tm timeinfo;
char timeStr[20];
int rssi;
char time_received[20];
char time_send[20];

// WiFiClient wifiClient;
// PubSubClient client(wifiClient);

void Display();
void LoRaSetUp();
void SetupWifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
void SendMessageToServer(int rssi);
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

  // if ( millis() - lastSend > 10000 ) { // Update and send only after 2 seconds
  //   SendMessageToServer();
  //   Serial.print("Send..");
  //   lastSend = millis();
  // }
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
// Fungsi untuk ekstraksi value JSON
String extractValue(String json, String key) {
  int start = json.indexOf("\"" + key + "\":");
  if (start == -1) return "";
  start = json.indexOf(":", start) + 1;
  int end;
  if (json[start] == '\"') {  // Value berupa string
    start++;
    end = json.indexOf("\"", start);
  } else {  // Value berupa angka
    end = json.indexOf(",", start);
    if (end == -1) end = json.indexOf("}", start);
  }
  return json.substring(start, end);
}

void LoRaReceiver() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Saat paket diterima
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
    }
    rssi = LoRa.packetRssi();
    Serial.print("Received packet: ");
    Serial.println(LoRaData);
    Serial.print("RSSI: ");
    Serial.println(rssi);

    // Dapatkan waktu saat data diterima
    if (getLocalTime(&timeinfo)) {
      sprintf(time_received, "%04d-%02d-%02d %02d:%02d:%02d", 
              timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    // Buat JSON baru untuk dikirim ke server MQTT
    sprintf(jsonData, 
            "{\"devUI\":\"%s\",\"time_at_device\":\"%s\","
            "\"time_received_at_gateway\":\"%s\","
            "\"RSSI\":%d,\"protocol\":\"LoRa\",\"data\":%s}", 
            extractValue(LoRaData, "devUI").c_str(), 
            extractValue(LoRaData, "time_at_device").c_str(),
            time_received,
            rssi,
            extractValue(LoRaData, "data").c_str());

    Serial.print("Data JSON: ");
    Serial.println(jsonData);

    // Persiapan waktu saat data akan dikirim ke server
    if (getLocalTime(&timeinfo)) {
      sprintf(time_send, "%04d-%02d-%02d %02d:%02d:%02d", 
              timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
    SendMessageToServer(rssi);
  }
}

void SendMessageToServer(int rssi) {
  // Ambil value dari JSON LoRaData
  String devUI = extractValue(LoRaData, "devUI");
  String timeAtDevice = extractValue(LoRaData, "time_at_device");
  String timeReceivedAtGateway = extractValue(LoRaData, "time_received_at_gateway");
  String timeSendFromGateway = extractValue(LoRaData, "time_send_from_gateway");
  String protocol = "mqtt";
  
  String voltage = extractValue(LoRaData, "voltage");
  String current = extractValue(LoRaData, "current");
  String power = extractValue(LoRaData, "power");
  String energy = extractValue(LoRaData, "energy");
  String frequency = extractValue(LoRaData, "frequency");
  String powerFactor = extractValue(LoRaData, "power_factor");

  // Tambahkan ke payload Antares satu per satu (tanpa objek bersarang)
  antares.add("devUI", devUI);
  antares.add("time_at_device", timeAtDevice);
  antares.add("time_received_at_gateway", time_received);
  antares.add("time_send_from_gateway", time_send);
  antares.add("RSSI", rssi);
  antares.add("protocol", protocol);
  
  antares.add("voltage", voltage);
  antares.add("current", current);
  antares.add("power", power);
  antares.add("energy", energy);
  antares.add("frequency", frequency);
  antares.add("power_factor", powerFactor);

  antares.publish(projectName, deviceName);
  Serial.println("Data sent to MQTT server.");
}

void updateTime() {
  if (getLocalTime(&timeinfo)) {
    Serial.println("Waktu diperbarui dari NTP!");
    sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    Serial.print("time: ");
    Serial.println(timeStr);
  } else {
    Serial.println("Gagal mendapatkan waktu dari NTP");
  }
  
}

