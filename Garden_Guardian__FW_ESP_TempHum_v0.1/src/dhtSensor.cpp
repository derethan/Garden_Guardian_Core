#include "dhtSensor.h"

DHTSensor::DHTSensor(uint8_t pin, uint8_t type) : pin(pin), type(type) {
    dht = new DHT_Unified(pin, type);
    delayMS = 0;
}

DHTSensor::~DHTSensor() {
    delete dht;
}

bool DHTSensor::begin() {
    dht->begin();
    sensor_t sensor;
    dht->temperature().getSensor(&sensor);
    delayMS = sensor.min_delay / 1000;
    return checkConnection();
}

bool DHTSensor::checkConnection() {
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    return !isnan(event.temperature);
}

float DHTSensor::readTemperature() {
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    return isnan(event.temperature) ? -999.0 : event.temperature;
}

float DHTSensor::readHumidity() {
    sensors_event_t event;
    dht->humidity().getEvent(&event);
    return isnan(event.relative_humidity) ? -999.0 : event.relative_humidity;
}

void DHTSensor::printSensorDetails() {
    sensor_t sensor;
    
    // Print temperature sensor details
    dht->temperature().getSensor(&sensor);
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor"));
    Serial.print(F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print(F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
    Serial.print(F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
    Serial.print(F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));
    
    // Print humidity sensor details
    dht->humidity().getSensor(&sensor);
    Serial.println(F("Humidity Sensor"));
    Serial.print(F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print(F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
    Serial.print(F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
    Serial.print(F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
    Serial.println(F("------------------------------------"));
}
