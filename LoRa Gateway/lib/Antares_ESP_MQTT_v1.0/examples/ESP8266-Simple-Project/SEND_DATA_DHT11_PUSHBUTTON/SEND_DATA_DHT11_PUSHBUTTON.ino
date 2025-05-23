// Include necessary libraries
#include <AntaresESPMQTT.h>  // Library initiation for Antares ESP8266 MQTT
#include "DHT.h"             // Include the DHT sensor library

#define DHTPIN D1      // Define the pin number for the DHT sensor (connected to pin 14)
#define DHTTYPE DHT11  // Define the DHT sensor type as DHT11

#define pushButtonPin A0  // Define the pin number for the push button (connected to pin A0)
#define ledPin D2         // Define the pin number for the LED (connected to pin D2)

#define ACCESSKEY "YOUR-ACCESS-KEY"    // Replace with your Antares account access key
#define WIFISSID "YOUR-WIFI-SSID"      // Replace with your Wi-Fi SSID
#define PASSWORD "YOUR-WIFI-PASSWORD"  // Replace with your Wi-Fi password

#define projectName "YOUR-APPLICATION-NAME"  // Replace with the Antares application name that was created
#define deviceName "YOUR-DEVICE-NAME"        // Replace with the Antares device name that was created

AntaresESPMQTT antares(ACCESSKEY);  // Create an object for interacting with Antares
DHT dht(DHTPIN, DHTTYPE);           // Create a DHT object for the DHT sensor

bool ledState = false;         // Variable to store the state of the LED (ON or OFF)
bool lastButtonState = false;  // Variable to store the previous state of the push button

void setup() {
  Serial.begin(115200);                        // Initialize serial communication with baudrate 115200
  antares.setDebug(true);                      // Enable debug mode for AntaresESP8266HTTP (messages will be displayed in the serial monitor)
  antares.wifiConnection(WIFISSID, PASSWORD);  // Connect to Wi-Fi with the specified SSID and password
  antares.setMqttServer();
  dht.begin();  // Initialize the DHT sensor

  pinMode(pushButtonPin, INPUT);  // Set the push button pin as input with internal pull-up resistor
  pinMode(ledPin, OUTPUT);        // Set the LED pin as output
  digitalWrite(ledPin, LOW);      // Turn off the LED initially
}

void loop() {
  antares.checkMqttConnection();
  // Read the current state of the push button
  int analogValue = analogRead(A0);
  bool currentButtonState = (analogValue < 512);  // Adjust the threshold as needed

  // Check if the button state has changed (button press detected)
  if (currentButtonState != lastButtonState && currentButtonState == LOW) {
    Serial.println("The button is pressed");

    // Toggle state of the LED (ON to OFF or OFF to ON)
    ledState = !ledState;
    digitalWrite(ledPin, ledState);

    // Read temperature and humidity from the DHT sensor
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();

    // Check if the sensor reading is valid
    if (isnan(hum) || isnan(temp)) {
      Serial.println("Failed to read DHT sensor!");
      return;
    }

    // Print temperature and humidity values to the serial monitor
    Serial.println("Temperature: " + (String)temp + " *C");
    Serial.println("Humidity: " + (String)hum + " %");

    // Add temperature and humidity data to the Antares storage buffer
    antares.add("temperature", temp);
    antares.add("humidity", hum);

    // Send the data from the storage buffer to Antares
    antares.publish(projectName, deviceName);

    // Add some delay to avoid multiple data sending due to button debouncing
    delay(1000);
  }

  // Update the last button state for the next iteration
  lastButtonState = currentButtonState;
}
