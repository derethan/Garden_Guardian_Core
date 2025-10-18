#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// OneWire bus pin definition
#define ONE_WIRE_BUS 0

// Setup OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Function declarations
void scanDS18B20Sensors();
void printTemperatureReadings();

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
    delay(5000);

  Serial.println("ESP32 DS18B20 Temperature Sensor Scanner");

  // Initialize the Dallas Temperature library
  sensors.begin();

  // Scan for DS18B20 sensors on the OneWire bus
  scanDS18B20Sensors();
}

void loop()
{
  // Print temperature readings every 5 seconds
  printTemperatureReadings();
  delay(5000);
}

// Function to scan for DS18B20 sensors on the OneWire bus
void scanDS18B20Sensors()
{
  Serial.println("\n--- Scanning for DS18B20 Temperature Sensors ---");
  Serial.print("OneWire Bus Pin: ");
  Serial.println(ONE_WIRE_BUS);

  // Get the number of devices found on the bus
  int deviceCount = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" device(s) on the OneWire bus");

  if (deviceCount == 0)
  {
    Serial.println("No DS18B20 sensors detected!");
    Serial.println("Check wiring and connections:");
    Serial.println("- VCC to 3.3V or 5V");
    Serial.println("- GND to Ground");
    Serial.println("- Data to GPIO 23 with 4.7kΩ pull-up resistor");
    return;
  }

  // Scan each device and print its ROM address
  DeviceAddress deviceAddress;
  for (int i = 0; i < deviceCount; i++)
  {
    if (sensors.getAddress(deviceAddress, i))
    {
      Serial.print("Sensor ");
      Serial.print(i + 1);
      Serial.print(" ROM Address: ");

      // Print the ROM address in hexadecimal format
      for (uint8_t j = 0; j < 8; j++)
      {
        if (deviceAddress[j] < 16)
          Serial.print("0");
        Serial.print(deviceAddress[j], HEX);
        if (j < 7)
          Serial.print(":");
      }
      Serial.println();

      // Check if the device is a DS18B20
      if (deviceAddress[0] == 0x28)
      {
        Serial.println("  -> Confirmed DS18B20 sensor");
      }
      else
      {
        Serial.println("  -> Unknown OneWire device");
      }
    }
    else
    {
      Serial.print("Unable to find address for sensor ");
      Serial.println(i + 1);
    }
  }

  // Set resolution to 12-bit (0.0625°C precision)
  sensors.setResolution(12);
  Serial.println("Sensor resolution set to 12-bit");
  Serial.println("--- Scan Complete ---\n");
}

// Function to read and print temperature from all sensors
void printTemperatureReadings()
{
  // Request temperature readings from all sensors
  sensors.requestTemperatures();

  int deviceCount = sensors.getDeviceCount();

  Serial.println("=== Temperature Readings ===");
  Serial.print("Timestamp: ");
  Serial.println(millis());

  for (int i = 0; i < deviceCount; i++)
  {
    float tempC = sensors.getTempCByIndex(i);

    // Check if reading is valid
    if (tempC != DEVICE_DISCONNECTED_C)
    {
      float tempF = sensors.getTempFByIndex(i);

      Serial.print("Sensor ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(tempC);
      Serial.print("°C (");
      Serial.print(tempF);
      Serial.println("°F)");
    }
    else
    {
      Serial.print("Sensor ");
      Serial.print(i + 1);
      Serial.println(": ERROR - Could not read temperature data");
    }
  }
  Serial.println("============================\n");
}
