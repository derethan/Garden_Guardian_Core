#ifndef TEMPSENSORS_H
#define TEMPSENSORS_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

class TempSensors {
public:
    TempSensors();
    void initialize();
    float readSensor1();
    float readSensor2();
    float readSensor3();
    bool isSensorConnected(int sensorIndex);
    void requestTemperatures();
    
private:
    OneWire oneWire1;
    OneWire oneWire2;
    OneWire oneWire3;
    DallasTemperature sensors1;
    DallasTemperature sensors2;
    DallasTemperature sensors3;
    bool sensor1Connected;
    bool sensor2Connected;
    bool sensor3Connected;
    float lastTemp1;
    float lastTemp2;
    float lastTemp3;
    const float TEMP_ERROR_VALUE = -127.0;
};

#endif // TEMPSENSORS_H
