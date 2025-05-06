#include <Arduino.h>
#include <PZEM004Tv30.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <SPI.h>
#include <LoRa.h>
#include "time.h"

// START DISPLAY PREQUISITE
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET    -1    // Reset tidak digunakan
#define SCREEN_ADDRESS 0x3C // Alamat I2C default  //   QT-PY / XIAO
Adafruit_SH1106G oled = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN) && !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17

#endif

#if defined(USE_SOFTWARE_SERIAL)
//SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
//PZEM004Tv30 pzem(pzemSWSerial);

#elif defined(ESP32)
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);
#else
PZEM004Tv30 pzem(PZEM_SERIAL);

#endif

#define ss 5
#define rst 14
#define dio0 2

unsigned long lastSend;
unsigned long lastOn;
int led = 2;
int counter =0;

float voltage, current, power, energy, frequency, pf, rssi;
double lora_freq = 433E6;
String chipID;
char chipIDStr[20];

const int MSG_BUFFER_SIZE = 600;
char data[MSG_BUFFER_SIZE];
const char* jsonData = "{\"devUI\":\"%s\",\"time_at_device\":\"%s\",\"frequency\":%.1f,\"data\":{"
                              "\"voltage\":%.2f,\"current\":%.2f,\"power\":%.2f,\"energy\":%.2f,"
                              "\"frequency\":%.2f,\"power_factor\":%.2f}}";

String ssid = "X282 2G";
const char* password = "itssurabaya";
const char* ntpServer = "pool.ntp.org"; // Server NTP
const long  gmtOffset_sec = 7 * 3600;   // GMT+7 untuk WIB
const int   daylightOffset_sec = 0;     // Tidak ada daylight saving di Indonesia
struct tm timeinfo;
char timeStr[20];
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void LoRaSetUp();
void LoRaSend();
void ReadPzem();
void PzemMonitor();
void updateTime();
void setup_wifi();
void displayData(float power, float energy, float voltage, float current, float frequency, float pf, String time);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  LoRaSetUp();
  pinMode(led, OUTPUT);

  setup_wifi();
  /// Konfigurasi NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Menunggu sinkronisasi waktu...");

  // Ambil waktu pertama kali
  updateTime();
  
  if (!oled.begin(SCREEN_ADDRESS, true)) { // Gunakan alamat I2C 0x3C dan reset internal
    Serial.println(F("SH1106 allocation failed"));
    for (;;);
  }

  oled.display();
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(46, 16);
  oled.println("AMI");
  oled.setCursor(28, 32);
  oled.println("Module");
  oled.drawLine(0, 8, 127, 8, SH110X_WHITE);
  oled.drawLine(0, 55, 127, 55, SH110X_WHITE);
  oled.display();
  delay(3500);
}

void loop() {
  // put your main code here, to run repeatedly:
  ReadPzem();
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  // PzemMonitor();
  if (millis() - lastSend > 10000) {
    updateTime();
    LoRaSend();
    Serial.print("Send..");
    lastSend = millis();
  }
}

void LoRaSetUp() {
  while (!Serial);
  Serial.println("AMI Sender");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(lora_freq)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF5);
  Serial.println("LoRa Initializing OK!");
}

void LoRaSend() {
  Serial.println("Sending packet: ");
  Serial.println(data);

  //Send LoRa packet to receiver
  LoRa.beginPacket();
  // LoRa.print("data : ");
  LoRa.print(data);
  LoRa.endPacket();
  counter++;

  // delay(1000);
}

void ReadPzem() {
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();

  displayData(power, energy, voltage, current, frequency, pf, timeStr);
  // Mendapatkan MAC address
  chipID = WiFi.macAddress();
  Serial.print("Chip ID (MAC Address): ");
  Serial.println(chipID);
  strcpy(chipIDStr, chipID.c_str());  // Konversi String ke char[]  // Konversi int ke char[]

  snprintf(data, MSG_BUFFER_SIZE, jsonData, chipIDStr, timeStr, lora_freq, voltage, current, power, energy, frequency, pf);
}

void PzemMonitor() {
   Serial.println("Pzem sensor read.");
  if(isnan(voltage)){
        Serial.println("Error reading voltage");
    } else if (isnan(current)) {
        Serial.println("Error reading current");
    } else if (isnan(power)) {
        Serial.println("Error reading power");
    } else if (isnan(energy)) {
        Serial.println("Error reading energy");
    } else if (isnan(frequency)) {
        Serial.println("Error reading frequency");
    } else if (isnan(pf)) {
        Serial.println("Error reading power factor");
    } else {

        // Print the values to the Serial console
        Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
        Serial.print("Current: ");      Serial.print(current);      Serial.println("A");
        Serial.print("Power: ");        Serial.print(power);        Serial.println("W");
        Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println("kWh");
        Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
        Serial.print("PF: ");           Serial.println(pf);
    }
    Serial.println();
}

void updateTime() {
  if (getLocalTime(&timeinfo)) {
    Serial.println("Waktu diperbarui dari NTP!");
    sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println("Gagal mendapatkan waktu dari NTP");
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Wifi Not Connected");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void displayData(float power, float energy, float voltage, float current, float frequency, float pf, String time) {
  oled.clearDisplay();
  oled.setTextSize(1); // Ukuran teks normal
  oled.setTextColor(SH110X_WHITE);

  int roundedPower = isnan(power) ? 0 : round(power);
  // int roundedVoltage = isnan(voltage) ? 0 : round(voltage);
  int roundedFrequency = isnan(frequency) ? 0 : round(frequency);
  
  float formattedEnergy = isnan(energy) ? 0.0 : floor(energy * 100) / 100.0;
  float roundedVoltage = isnan(voltage) ? 0.0 : floor(voltage * 100) / 100.0;
  float formattedCurrent = isnan(current) ? 0.0 : floor(current * 100) / 100.0;
  float formattedPF = isnan(pf) ? 0.0 : floor(pf * 100) / 100.0;
  
  oled.setCursor(0, 0);
  oled.print("Power    : "); oled.print(roundedPower); 
  oled.setCursor(102, 0);
  oled.println("W");

  oled.setCursor(0, 8);
  oled.print("Energy   : "); oled.print(formattedEnergy);
  oled.setCursor(102, 8);
  oled.println("kWh");

  oled.setCursor(0, 16);
  oled.print("Voltage  : "); oled.print(roundedVoltage);
  oled.setCursor(102, 16);
  oled.println("V");

  oled.setCursor(0, 24);
  oled.print("Current  : "); oled.print(formattedCurrent); 
  oled.setCursor(102, 24);
  oled.println("Amp");

  oled.setCursor(0, 32);
  oled.print("Frequency: "); oled.print(roundedFrequency); 
  oled.setCursor(102, 32);
  oled.println("Hz");

  oled.setCursor(0, 40);
  oled.print("PF       : "); oled.println(formattedPF);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Wifi Not Connected");
    oled.setCursor(0, 48);
    oled.println("Time     : "); oled.println("Wifi Not Connected");
  }else {
    oled.setCursor(0, 48);
    oled.println("Time     : "); oled.println(time);
  }

  oled.display();
}
