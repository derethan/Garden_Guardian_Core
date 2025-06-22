#ifndef TEMPSENSORS_H
#define TEMPSENSORS_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

#define MAX_DS18B20_SENSORS 8

class TempSensors
{
public:
    TempSensors();
    void initialize();
    // Read temperature from a specific sensor by index
    float readSensor(uint8_t index = 0);
    // Read all sensors, store results in provided array, returns number of sensors read
    int readAllSensors(float *temps, int maxCount);
    // Get the number of sensors detected
    uint8_t getSensorCount() const;
    // Check if a specific sensor is connected
    bool isSensorConnected(uint8_t index = 0) const;
    // Get the address of a sensor by index
    bool getSensorAddress(uint8_t index, DeviceAddress address) const;

private:
    OneWire oneWire;
    DallasTemperature sensors;
    DeviceAddress sensorAddresses[MAX_DS18B20_SENSORS];
    uint8_t sensorCount;
    bool sensorConnected[MAX_DS18B20_SENSORS];
    float lastTemp[MAX_DS18B20_SENSORS];
};

#endif // TEMPSENSORS_H
