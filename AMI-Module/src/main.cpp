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

float voltage, current, power, energy, frequency, pf;
String chipID;

#define MSG_BUFFER_SIZE  (500)
char data[MSG_BUFFER_SIZE] = "{\"DevUI\":\"%.2f\",\"Voltage_PLN\":\"%.2f\",\"Current_PLN\":\"%.2f\",\"Power_PLN\":\"%.2f\",\"Energy_PLN\":\"%.2f\",\"Frekuensi_PLN\":\"%.2f\",\"Power_Faktor_PLN\":\"%.2f\"}";
char data_buffer[500];

const char* ssid = "crustea";
const char* password = "crustea1234";
const char* ntpServer = "pool.ntp.org"; // Server NTP
const long  gmtOffset_sec = 7 * 3600;   // GMT+7 untuk WIB
const int   daylightOffset_sec = 0;     // Tidak ada daylight saving di Indonesia
struct tm timeinfo;

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
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF5);
  Serial.println("LoRa Initializing OK!");
}

void LoRaSend() {
  Serial.println("Sending packet: ");
  Serial.println(data_buffer);

  //Send LoRa packet to receiver
  LoRa.beginPacket();
  // LoRa.print("data : ");
  LoRa.print(data_buffer);
  LoRa.endPacket();
  counter++;

  delay(1000);
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

  sprintf(data_buffer, data, chipID, voltage, current, power, energy, frequency, pf);
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