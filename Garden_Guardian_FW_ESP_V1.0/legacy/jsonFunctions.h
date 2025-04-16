
#ifndef JSONFUNCTIONS_H
#define JSONFUNCTIONS_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct sensorData {
    String name;
    String sensorName;
    unsigned long timestamp;
    String sensorType;
    String sensorLocation;
    String dataType;
    float data;
};

class JsonFunctions {
public:
    static String convertToJSON(sensorData* data, int size, const String& device_id);
    static void addSensorReading(JsonArray& SensorReadings, const sensorData& sensor);
};

#endif // JSONFUNCTIONS_H