#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin konfigurasi LoRa (sesuai yang kamu pakai)
#define ss    5
#define rst   14
#define dio0  2

#define LORA_FREQUENCY 868E6  // Ganti sesuai kebutuhan: 433E6, 868E6, 915E6

void tampilkanStatus(String pesan) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(pesan);
  display.setCursor(0, 9);
  display.setTextSize(2);
  display.println(String(LORA_FREQUENCY / 1E6) + " MHz");
  display.display();
}

void writeLoRaRegister(uint8_t address, uint8_t value) {
  digitalWrite(ss, LOW);
  SPI.transfer(address | 0x80);  // Write mode
  SPI.transfer(value);
  digitalWrite(ss, HIGH);
}

void loraContinuousWave() {
  LoRa.idle(); // Masuk standby

  delay(10);
  writeLoRaRegister(0x01, 0x00); // RegOpMode = standby FSK
  delay(10);
  writeLoRaRegister(0x01, 0x83); // RegOpMode = TX continuous wave

  Serial.println("Transmitting CW...");
  tampilkanStatus("Transmitting CW...");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Gagal inisialisasi OLED");
    while (1);
  }
  display.clearDisplay();
  tampilkanStatus("Mulai LoRa...");

  // SPI init
  SPI.begin();  // Default: SCK=18, MISO=19, MOSI=23

  // LoRa pin setup
  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    tampilkanStatus("LoRa gagal!");
    while (1);
  }

  tampilkanStatus("CW Mode\nFrekuensi:\n" );

  // Aktifkan CW
  loraContinuousWave();
}

void loop() {
  // Tidak perlu loop aktif
}
