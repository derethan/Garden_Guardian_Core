#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Maximum number of sensors supported
#define MAX_SENSORS 10

// Structure to store sensor information
struct SensorInfo {
  DeviceAddress address;  // 8-byte ROM address
  String name;           // User-friendly name
  bool isActive;         // Whether sensor is currently connected
  int sensorIndex;       // Index for Dallas library functions
};

class TemperatureSensorManager {
private:
  OneWire* oneWire;
  DallasTemperature* sensors;
  SensorInfo registeredSensors[MAX_SENSORS];
  int sensorCount;
  int oneWireBusPin;

  // Helper functions
  String addressToString(DeviceAddress address);
  bool addressesMatch(DeviceAddress addr1, DeviceAddress addr2);
  int registerSensor(DeviceAddress address, int index);

public:
  // Constructor
  TemperatureSensorManager(int busPin);
  
  // Destructor
  ~TemperatureSensorManager();

  // Public methods
  void begin();
  void scanSensors();
  void printTemperatureReadings();
  void printSensorInfo();
  void renameSensor(int sensorId, String newName);
  int findSensorById(int sensorId);
  int getSensorCount();
  bool isSensorActive(int sensorId);
  String getSensorName(int sensorId);
  String getSensorAddress(int sensorId);
  float getTemperatureC(int sensorId);
  float getTemperatureF(int sensorId);
};

#endif // TEMPERATURE_SENSOR_H