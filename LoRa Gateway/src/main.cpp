#include <Arduino.h>
// #include <PubSubClient.h>
// #include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include <AntaresESPMQTT.h>
#include <AntaresESPHTTP.h>
#include "time.h"

// START DISPLAY PREQUISITE
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Ganti alamat I2C sesuai dengan hasil scan
#define OLED_ADDR 0x3C

// Inisialisasi LCD dan OLED
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define ACCESSKEY "fbcc54e8fa44769a:601102b2dae28971"       // Antares account access key
#define WIFISSID "X282 2G"         // Wi-Fi SSID to connect to
#define PASSWORD "itssurabaya"     // Wi-Fi password

#define projectName "AMIModule"   // Name of the application created in Antares
#define deviceName "LoRa_Gateway"     // Name of the device created in Antares

// Inisialisasi Objek Antares
AntaresESPHTTP antaresHTTP(ACCESSKEY);
AntaresESPMQTT antaresMQTT(ACCESSKEY);

WiFiClient myClient;

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

String ssid = "X282 2G";
const char* password = "itssurabaya";
const char* ntpServer = "pool.ntp.org"; // Server NTP
const long  gmtOffset_sec = 7 * 3600;   // GMT+7 untuk WIB
const int   daylightOffset_sec = 0;     // Tidak ada daylight saving di Indonesia
char status_koneksi[8] = "Offline";
unsigned long lastClockUpdate = 0;
const unsigned long clockInterval = 1000; // 1 detik

bool mainScreenDrawn = false;
String LoRaData;
struct tm timeinfo;
char timeStr[20];
int rssi;
char time_received[20];
char time_send[30];
unsigned long lastSwitchTime = 0;
unsigned long rebootInterval = 86400000;
const long switchInterval = 30000; // Interval pergantian 30 detik
bool useMQTT = true;
String protocol = "MQTT";
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
void displayMainScreen(String deviceType, String wifiStatus, String ssid, String ntpTime, String lastReceived);
void displayJsonData(String jsonData, String protocol);
void updateClockDisplay(String ntpTime);

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
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  oled.getTextBounds("LoRa Gateway", 0, 0, &x1, &y1, &w, &h);

  int x = (oled.width() - w) / 2;
  int y = (oled.height() - h) / 2;

  oled.drawLine(0, 8, 127, 8, SSD1306_WHITE);

  oled.setCursor(45, 16);

  oled.println("LoRa");
  oled.setCursor(27, 32);
  oled.println("Gateway");

  oled.drawLine(0, 55, 127, 55, SSD1306_WHITE);

  // Tampilkan perubahan pada layar
  oled.display();

  antaresHTTP.wifiConnection(WIFISSID, PASSWORD);
  antaresMQTT.wifiConnection(WIFISSID, PASSWORD);
  antaresMQTT.setMqttServer();

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
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(WIFISSID, PASSWORD);
    strcpy(status_koneksi, "Offline");
    mainScreenDrawn = false;
  } else {
    strcpy(status_koneksi, "Connected");
  }

  antaresMQTT.checkMqttConnection();

  displayMainScreen("LoRa Gateway", status_koneksi, WIFISSID, timeStr, time_received);

  if (!mainScreenDrawn) {
    displayMainScreen("LoRa Gateway", status_koneksi, WIFISSID, timeStr, time_received);
    mainScreenDrawn = true;
  }

  // Realtime clock update per detik
  if (millis() - lastClockUpdate >= clockInterval) {
    updateTime();  // perbarui timeStr
    updateClockDisplay(timeStr);
    lastClockUpdate = millis();
  }

  if ( millis() - lastOn > rebootInterval ) { // Update and send only after 2 seconds
    // Serial.println("ESP Restarting...");  // Cetak pesan sebelum restart
    // Serial.flush();
    
    // oled.clearDisplay();
    // oled.setTextSize(2);
    // oled.setCursor(10,25);
    // oled.print("ESP Restarting...");
    // oled.display();
    
    // delay(2000);

    // ESP.restart();
    lastOn = millis();
  }
}

void LoRaSetUp() {
  while (!Serial);
  Serial.println("LoRa Gateway Started!!");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(915E6)) {
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
    displayJsonData(LoRaData, protocol);
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

    if (getLocalTime(&timeinfo)) {
      int millisec = millis() % 1000;  // Ambil milidetik
      sprintf(time_send, "%04d-%02d-%02d %02d:%02d:%02d.%03d", 
              timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, millisec);
      
    }
    SendMessageToServer(rssi);
  }
}

void SendMessageToServer(int rssi) {
  // Ambil value dari JSON LoRaData
  String devUI = extractValue(LoRaData, "devUI");
  String timeAtDevice = extractValue(LoRaData, "time_at_device");
  
  String voltage = extractValue(LoRaData, "voltage");
  String current = extractValue(LoRaData, "current");
  String power = extractValue(LoRaData, "power");
  String energy = extractValue(LoRaData, "energy");
  String frequency = extractValue(LoRaData, "frequency");
  String powerFactor = extractValue(LoRaData, "power_factor");

  // Cek apakah waktu untuk pergantian protokol
  if (millis() - lastSwitchTime > switchInterval) {
    lastSwitchTime = millis();
    useMQTT = !useMQTT;
  }

  // Kirim data menggunakan protokol yang aktif
  if (useMQTT) {
    antaresMQTT.add("devUI", devUI);
    antaresMQTT.add("time_at_device", timeAtDevice);
    antaresMQTT.add("time_received_at_gateway", time_received);
    antaresMQTT.add("time_send_from_gateway", time_send);
    antaresMQTT.add("RSSI", rssi);
    antaresMQTT.add("protocol", "mqtt");
    antaresMQTT.add("voltage", voltage);
    antaresMQTT.add("current", current);
    antaresMQTT.add("power", power);
    antaresMQTT.add("energy", energy);
    antaresMQTT.add("frequency", frequency);
    antaresMQTT.add("power_factor", powerFactor);
    

    antaresMQTT.publish(projectName, deviceName);
    Serial.println("Mengirim data dengan MQTT");
  } else {
    antaresHTTP.add("devUI", devUI);
    antaresHTTP.add("time_at_device", timeAtDevice);
    antaresHTTP.add("time_received_at_gateway", time_received);
    antaresHTTP.add("time_send_from_gateway", time_send);
    antaresHTTP.add("RSSI", rssi);
    antaresHTTP.add("protocol", "http");
    antaresHTTP.add("voltage", voltage);
    antaresHTTP.add("current", current);
    antaresHTTP.add("power", power);
    antaresHTTP.add("energy", energy);
    antaresHTTP.add("frequency", frequency);
    antaresHTTP.add("power_factor", powerFactor);

    antaresHTTP.send(projectName, deviceName);
    Serial.println("Mengirim data dengan HTTP");
  }
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

void displayMainScreen(String deviceType, String wifiStatus, String ssid, String ntpTime, String lastReceived) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(25, 0);
  oled.println(deviceType);
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  oled.setCursor(0, 12);
  oled.print("WiFi  : ");
  oled.println(wifiStatus);

  oled.setCursor(0, 22);
  oled.print("SSID  : ");
  oled.println(ssid);

  oled.setCursor(0, 32);
  oled.print("Time  : ");
  String formattedTime = ntpTime.substring(11, 19);
  oled.println(formattedTime);

  oled.setCursor(0, 42);
  oled.println("Last Data: ");
  oled.setCursor(0, 52);
  oled.println(lastReceived);

  oled.drawLine(0, 63, 127, 63, SSD1306_WHITE);
  oled.display();
}

void displayJsonData(String jsonData, String protocol) {
  
  String devUI = extractValue(LoRaData, "devUI");
  String timeAtDevice = extractValue(LoRaData, "time_at_device");
  
  String voltage = extractValue(LoRaData, "voltage");
  String current = extractValue(LoRaData, "current");
  String power = extractValue(LoRaData, "power");
  String energy = extractValue(LoRaData, "energy");
  String frequency = extractValue(LoRaData, "frequency");
  String powerFactor = extractValue(LoRaData, "power_factor");
  String formatedVoltage = voltage.substring(0,3);

  oled.clearDisplay();
  oled.setCursor(25, 0);
  oled.setTextSize(1);
  oled.println("Data Received");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setCursor(0, 12);
  oled.println("DevUI: ");
  oled.setCursor(0, 22);
  oled.println(devUI);
  oled.setCursor(0, 32);
  oled.print("Time Device: ");
  String formattedTime = timeAtDevice.substring(11, 19);
  oled.println(formattedTime);
  oled.setCursor(0, 42);
  oled.print("RSSI: ");
  oled.print(rssi); oled.print(", P: "); oled.print(power); oled.println(" W");
  oled.setCursor(0, 52);
  oled.print("V   : "); oled.print(formatedVoltage); oled.print("V");
  oled.print(", C: "); oled.print(current); oled.println(" A");
  oled.drawLine(0, 62, 127, 62, SSD1306_WHITE);
  oled.display();
  delay(5000); // Tampilkan sementara
}

void updateClockDisplay(String ntpTime) {
  String formattedTime = ntpTime.substring(11, 19);
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.fillRect(48, 32, 80, 8, BLACK); // clear area jam sebelumnya
  oled.setCursor(48, 32);              // posisi tetap agar tidak bergerak
  oled.println(formattedTime);
  oled.display();
}