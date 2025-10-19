#include <Arduino.h>
#include "TemperatureSensor.h"

// OneWire bus pin definition
#define ONE_WIRE_BUS 0

// pin 2 is led
#define LED_PIN 2

// Create temperature sensor manager instance
TemperatureSensorManager tempSensorManager(ONE_WIRE_BUS);

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  delay(5000);

  Serial.println("ESP32 DS18B20 Temperature Sensor Scanner");

  // Initialize the temperature sensor manager
  tempSensorManager.begin();

  // Scan for DS18B20 sensors on the OneWire bus
  tempSensorManager.scanSensors();

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


}

void loop()
{

  // Print temperature readings every 5 seconds
  tempSensorManager.printTemperatureReadings();
  delay(5000);
}
