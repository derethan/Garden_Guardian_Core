#include "TemperatureSensor.h"

// Constructor
TemperatureSensorManager::TemperatureSensorManager(int busPin) {
  oneWireBusPin = busPin;
  oneWire = new OneWire(busPin);
  sensors = new DallasTemperature(oneWire);
  sensorCount = 0;
  
  // Initialize sensor array
  for (int i = 0; i < MAX_SENSORS; i++) {
    registeredSensors[i].isActive = false;
    registeredSensors[i].name = "";
    registeredSensors[i].sensorIndex = -1;
  }
}

// Destructor
TemperatureSensorManager::~TemperatureSensorManager() {
  delete sensors;
  delete oneWire;
}

// Initialize the temperature sensor system
void TemperatureSensorManager::begin() {
  Serial.println("Initializing Temperature Sensor Manager...");
  sensors->begin();
  Serial.print("OneWire Bus Pin: ");
  Serial.println(oneWireBusPin);
}

// Helper function to convert device address to string
String TemperatureSensorManager::addressToString(DeviceAddress address) {
  String result = "";
  for (uint8_t i = 0; i < 8; i++) {
    if (address[i] < 16) result += "0";
    result += String(address[i], HEX);
    if (i < 7) result += ":";
  }
  result.toUpperCase();
  return result;
}

// Helper function to compare two addresses
bool TemperatureSensorManager::addressesMatch(DeviceAddress addr1, DeviceAddress addr2) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr1[i] != addr2[i]) return false;
  }
  return true;
}

// Register a new sensor or update existing one
int TemperatureSensorManager::registerSensor(DeviceAddress address, int index) {
  // Check if sensor already exists
  for (int i = 0; i < sensorCount; i++) {
    if (addressesMatch(registeredSensors[i].address, address)) {
      // Update existing sensor
      registeredSensors[i].isActive = true;
      registeredSensors[i].sensorIndex = index;
      Serial.print("Updated existing sensor ID ");
      Serial.println(i);
      return i;
    }
  }
  
  // Add new sensor if we have space
  if (sensorCount < MAX_SENSORS) {
    // Copy address
    for (uint8_t i = 0; i < 8; i++) {
      registeredSensors[sensorCount].address[i] = address[i];
    }
    
    registeredSensors[sensorCount].name = "Sensor_" + String(sensorCount);
    registeredSensors[sensorCount].isActive = true;
    registeredSensors[sensorCount].sensorIndex = index;
    
    Serial.print("Registered new sensor with ID ");
    Serial.println(sensorCount);
    
    return sensorCount++;
  }
  
  Serial.println("Error: Maximum sensors reached!");
  return -1;
}

// Scan for DS18B20 sensors on the OneWire bus
void TemperatureSensorManager::scanSensors() {
  Serial.println("\n--- Scanning for DS18B20 Temperature Sensors ---");
  Serial.print("OneWire Bus Pin: ");
  Serial.println(oneWireBusPin);

  // Mark all sensors as inactive first
  for (int i = 0; i < sensorCount; i++) {
    registeredSensors[i].isActive = false;
  }

  // Get the number of devices found on the bus
  int deviceCount = sensors->getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" device(s) on the OneWire bus");

  if (deviceCount == 0) {
    Serial.println("No DS18B20 sensors detected!");
    Serial.println("Check wiring and connections:");
    Serial.println("- VCC to 3.3V or 5V");
    Serial.println("- GND to Ground");
    Serial.print("- Data to GPIO ");
    Serial.print(oneWireBusPin);
    Serial.println(" with 4.7kΩ pull-up resistor");
    return;
  }

  // Scan each device and register it
  DeviceAddress deviceAddress;
  for (int i = 0; i < deviceCount; i++) {
    if (sensors->getAddress(deviceAddress, i)) {
      Serial.print("Device ");
      Serial.print(i + 1);
      Serial.print(" ROM Address: ");
      Serial.println(addressToString(deviceAddress));

      // Check if the device is a DS18B20
      if (deviceAddress[0] == 0x28) {
        Serial.println("  -> Confirmed DS18B20 sensor");
        int sensorId = registerSensor(deviceAddress, i);
        if (sensorId >= 0) {
          Serial.print("  -> Assigned ID: ");
          Serial.print(sensorId);
          Serial.print(" (");
          Serial.print(registeredSensors[sensorId].name);
          Serial.println(")");
        }
      } else {
        Serial.println("  -> Unknown OneWire device (not DS18B20)");
      }
    } else {
      Serial.print("Unable to find address for device ");
      Serial.println(i + 1);
    }
  }

  // Set resolution to 12-bit (0.0625°C precision)
  sensors->setResolution(12);
  Serial.println("Sensor resolution set to 12-bit");
  Serial.println("--- Scan Complete ---\n");
  
  // Print sensor registration summary
  printSensorInfo();
}

// Read and print temperature from all sensors
void TemperatureSensorManager::printTemperatureReadings() {
  // Request temperature readings from all sensors
  sensors->requestTemperatures();

  Serial.println("=== Temperature Readings ===");
  Serial.print("Timestamp: ");
  Serial.println(millis());

  bool hasActiveReadings = false;

  // Loop through registered sensors
  for (int i = 0; i < sensorCount; i++) {
    if (!registeredSensors[i].isActive) continue;
    
    hasActiveReadings = true;
    int sensorIndex = registeredSensors[i].sensorIndex;
    float tempC = sensors->getTempCByIndex(sensorIndex);

    // Check if reading is valid
    if (tempC != DEVICE_DISCONNECTED_C) {
      float tempF = sensors->getTempFByIndex(sensorIndex);

      Serial.print("ID ");
      Serial.print(i);
      Serial.print(" (");
      Serial.print(registeredSensors[i].name);
      Serial.print("): ");
      Serial.print(tempC);
      Serial.print("°C (");
      Serial.print(tempF);
      Serial.print("°F) [");
      Serial.print(addressToString(registeredSensors[i].address));
      Serial.println("]");
    } else {
      Serial.print("ID ");
      Serial.print(i);
      Serial.print(" (");
      Serial.print(registeredSensors[i].name);
      Serial.println("): ERROR - Could not read temperature data");
      
      // Mark sensor as inactive if it can't be read
      registeredSensors[i].isActive = false;
    }
  }
  
  if (!hasActiveReadings) {
    Serial.println("No active sensors found. Run scan to detect sensors.");
  }
  
  Serial.println("============================\n");
}

// Print information about all registered sensors
void TemperatureSensorManager::printSensorInfo() {
  Serial.println("\n=== Registered Sensors ===");
  for (int i = 0; i < sensorCount; i++) {
    Serial.print("ID ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(registeredSensors[i].name);
    Serial.print(" [");
    Serial.print(addressToString(registeredSensors[i].address));
    Serial.print("] - ");
    Serial.println(registeredSensors[i].isActive ? "Active" : "Inactive");
  }
  Serial.println("========================\n");
}

// Rename a sensor
void TemperatureSensorManager::renameSensor(int sensorId, String newName) {
  if (sensorId >= 0 && sensorId < sensorCount) {
    registeredSensors[sensorId].name = newName;
    Serial.print("Sensor ID ");
    Serial.print(sensorId);
    Serial.print(" renamed to: ");
    Serial.println(newName);
  } else {
    Serial.println("Error: Invalid sensor ID");
  }
}

// Find sensor by ID
int TemperatureSensorManager::findSensorById(int sensorId) {
  if (sensorId >= 0 && sensorId < sensorCount) {
    return sensorId;
  }
  return -1;
}

// Get total number of registered sensors
int TemperatureSensorManager::getSensorCount() {
  return sensorCount;
}

// Check if a sensor is active
bool TemperatureSensorManager::isSensorActive(int sensorId) {
  if (sensorId >= 0 && sensorId < sensorCount) {
    return registeredSensors[sensorId].isActive;
  }
  return false;
}

// Get sensor name
String TemperatureSensorManager::getSensorName(int sensorId) {
  if (sensorId >= 0 && sensorId < sensorCount) {
    return registeredSensors[sensorId].name;
  }
  return "Invalid ID";
}

// Get sensor address as string
String TemperatureSensorManager::getSensorAddress(int sensorId) {
  if (sensorId >= 0 && sensorId < sensorCount) {
    return addressToString(registeredSensors[sensorId].address);
  }
  return "Invalid ID";
}

// Get temperature in Celsius for specific sensor
float TemperatureSensorManager::getTemperatureC(int sensorId) {
  if (sensorId >= 0 && sensorId < sensorCount && registeredSensors[sensorId].isActive) {
    sensors->requestTemperatures();
    return sensors->getTempCByIndex(registeredSensors[sensorId].sensorIndex);
  }
  return DEVICE_DISCONNECTED_C;
}

// Get temperature in Fahrenheit for specific sensor
float TemperatureSensorManager::getTemperatureF(int sensorId) {
  if (sensorId >= 0 && sensorId < sensorCount && registeredSensors[sensorId].isActive) {
    sensors->requestTemperatures();
    return sensors->getTempFByIndex(registeredSensors[sensorId].sensorIndex);
  }
  return DEVICE_DISCONNECTED_F;
}