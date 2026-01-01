#include <Arduino.h>
#include "TemperatureSensor.h"
#include "NetworkManager.h"

// OneWire bus pin definition
#define ONE_WIRE_BUS 0

// pin 2 is led
#define LED_PIN 2

// Create temperature sensor manager instance
TemperatureSensorManager tempSensorManager(ONE_WIRE_BUS);

// Create network manager instance
NetworkManager networkManager(&tempSensorManager);

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  delay(5000);

  Serial.println("GG Water Temperature Monitor - Starting...");
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize the temperature sensor manager
  Serial.println("Initializing temperature sensors...");
  tempSensorManager.begin();
  tempSensorManager.scanSensors();

  // Initialize the network manager
  Serial.println("Initializing network manager...");
  networkManager.begin();

  // Example: Rename sensors if they exist
  // Uncomment these lines and modify as needed for your setup
  /*
  if (tempSensorManager.getSensorCount() > 0) {
    tempSensorManager.renameSensor(0, "Pool_Water");
  }
  if (tempSensorManager.getSensorCount() > 1) {
    tempSensorManager.renameSensor(1, "Ambient_Air");
  }
  */

  Serial.println("System initialization complete!");
  Serial.println("===============================");
  
  // LED indicator - system ready
  digitalWrite(LED_PIN, HIGH);
}

void loop()
{
  // Update network manager (handles reconnections, captive portal, etc.)
  networkManager.update();

  // Print temperature readings every 30 seconds
  static unsigned long lastTempReading = 0;
  if (millis() - lastTempReading > 30000) {
    tempSensorManager.printTemperatureReadings();
    lastTempReading = millis();
  }

  // LED blink pattern based on connection status
  static unsigned long lastLedBlink = 0;
  if (millis() - lastLedBlink > (networkManager.isWiFiConnected() ? 2000 : 500)) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastLedBlink = millis();
  }

  // Small delay to prevent watchdog issues
  delay(100);
}
