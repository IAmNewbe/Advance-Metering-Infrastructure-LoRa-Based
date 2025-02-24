#include <Arduino.h>
#include <PZEM004Tv30.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include "time.h"

// START DISPLAY PREQUISITE
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Ganti alamat I2C sesuai dengan hasil scan
#define OLED_ADDR 0x3C

// Inisialisasi LCD dan OLED
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

#define NUM_PZEMS 3
#define HEX 16

PZEM004Tv30 pzems[NUM_PZEMS];

/* *********************
 * Uncomment USE_SOFTWARE_SERIAL in order to enable Softare serial
 *
 * Does not work for ESP32
 ***********************/
//#define USE_SOFTWARE_SERIAL
#if defined(USE_SOFTWARE_SERIAL) && defined(ESP32)
    #error "Can not use SoftwareSerial with ESP32"
#elif defined(USE_SOFTWARE_SERIAL)

#include <SoftwareSerial.h>

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
#endif

PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN  , 0x01);

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

const char* ssid = "crustea";
const char* password = "crustea1234";
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
  
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  oled.display();
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Power Meter");
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

  WiFi.begin(ssid, password);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Wifi Not Connected");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}