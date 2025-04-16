#include "jsonFunctions.h"

String JsonFunctions::convertToJSON(sensorData* data, int size, const String& device_id) {
    JsonDocument doc;
    JsonArray Data = doc["Data"].to<JsonArray>();

    for (int i = 0; i < size; i++) {
        if (data[i].data != 0) {
            JsonObject sensorDataObject = Data.add<JsonObject>();
            JsonObject DeviceInfo = sensorDataObject["Device"].to<JsonObject>();
            DeviceInfo["DeviceID"] = device_id;

            JsonArray SensorReadings = sensorDataObject["SensorReadings"].to<JsonArray>();
            addSensorReading(SensorReadings, data[i]);
        }
    }

    String postData;
    serializeJson(doc, postData);
    return postData;
}

void JsonFunctions::addSensorReading(JsonArray& SensorReadings, const sensorData& sensor) {
    if (sensor.data != 0) {
        JsonObject reading = SensorReadings.add<JsonObject>();
        reading["Name"] = sensor.name;
        reading["Value"] = sensor.data;
        reading["Time"] = sensor.timestamp;

        if (!sensor.sensorName.isEmpty()) {
            reading["Sensor"] = sensor.sensorName;
        }
        if (!sensor.sensorType.isEmpty()) {
            reading["Type"] = sensor.sensorType;
        }
        if (!sensor.dataType.isEmpty()) {
            reading["Field"] = sensor.dataType;
        }
        if (!sensor.sensorLocation.isEmpty()) {
            reading["Location"] = sensor.sensorLocation;
        }
    }
}