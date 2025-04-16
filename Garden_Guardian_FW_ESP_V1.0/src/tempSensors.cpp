#include "tempSensors.h"

TempSensors::TempSensors() 
    : oneWire1(TEMP_SENSOR_1_PIN)
    , oneWire2(TEMP_SENSOR_2_PIN)
    , oneWire3(TEMP_SENSOR_3_PIN)
    , sensors1(&oneWire1)
    , sensors2(&oneWire2)
    , sensors3(&oneWire3)
    , sensor1Connected(false)
    , sensor2Connected(false)
    , sensor3Connected(false)
    , lastTemp1(TEMP_ERROR_VALUE)
    , lastTemp2(TEMP_ERROR_VALUE)
    , lastTemp3(TEMP_ERROR_VALUE)
{
}

void TempSensors::initialize() {
    sensors1.begin();
    sensors2.begin();
    sensors3.begin();
    
    // Check if sensors are connected
    sensors1.requestTemperatures();
    sensors2.requestTemperatures();
    sensors3.requestTemperatures();
    
    sensor1Connected = (sensors1.getTempCByIndex(0) != TEMP_ERROR_VALUE);
    sensor2Connected = (sensors2.getTempCByIndex(0) != TEMP_ERROR_VALUE);
    sensor3Connected = (sensors3.getTempCByIndex(0) != TEMP_ERROR_VALUE);
    
    if (DEBUG_MODE) {
        Serial.println("Temperature Sensors Initialization:");
        Serial.print("Sensor 1 Connected: "); Serial.println(sensor1Connected);
        Serial.print("Sensor 2 Connected: "); Serial.println(sensor2Connected);
        Serial.print("Sensor 3 Connected: "); Serial.println(sensor3Connected);
    }
}

void TempSensors::requestTemperatures() {
    if (sensor1Connected) sensors1.requestTemperatures();
    if (sensor2Connected) sensors2.requestTemperatures();
    if (sensor3Connected) sensors3.requestTemperatures();
}

float TempSensors::readSensor1() {
    if (!sensor1Connected) return TEMP_ERROR_VALUE;
    
    float temp = sensors1.getTempCByIndex(0);
    if (temp != TEMP_ERROR_VALUE) {
        lastTemp1 = temp;
        if (DEBUG_MODE) {
            Serial.print("Sensor 1 Temperature: ");
            Serial.println(temp);
        }
    }
    return lastTemp1;
}

float TempSensors::readSensor2() {
    if (!sensor2Connected) return TEMP_ERROR_VALUE;
    
    float temp = sensors2.getTempCByIndex(0);
    if (temp != TEMP_ERROR_VALUE) {
        lastTemp2 = temp;
        if (DEBUG_MODE) {
            Serial.print("Sensor 2 Temperature: ");
            Serial.println(temp);
        }
    }
    return lastTemp2;
}

float TempSensors::readSensor3() {
    if (!sensor3Connected) return TEMP_ERROR_VALUE;
    
    float temp = sensors3.getTempCByIndex(0);
    if (temp != TEMP_ERROR_VALUE) {
        lastTemp3 = temp;
        if (DEBUG_MODE) {
            Serial.print("Sensor 3 Temperature: ");
            Serial.println(temp);
        }
    }
    return lastTemp3;
}

bool TempSensors::isSensorConnected(int sensorIndex) {
    switch(sensorIndex) {
        case 1: return sensor1Connected;
        case 2: return sensor2Connected;
        case 3: return sensor3Connected;
        default: return false;
    }
}
