/**
 * @file dhtSensor.cpp
 * @brief DHT sensor wrapper implementation for temperature and humidity readings
 * 
 * Provides a wrapper around the DHT Unified Sensor library for reading
 * temperature and humidity data from DHT sensors (DHT11, DHT22, etc.).
 */

#include "dhtSensor.h"
#include "base/sysLogs.h"

/**
 * @brief Construct a new DHTSensor object
 * @param pin GPIO pin number connected to the DHT sensor
 * @param type DHT sensor type (DHT11, DHT22, etc.)
 */
DHTSensor::DHTSensor(uint8_t pin, uint8_t type) : pin(pin), type(type) {
    dht = new DHT_Unified(pin, type);
    delayMS = 0;
}

/**
 * @brief Destroy the DHTSensor object and free allocated resources
 */
DHTSensor::~DHTSensor() {
    delete dht;
}

/**
 * @brief Initialize the DHT sensor and get minimum delay between readings
 * @return true if initialization successful, false otherwise
 */
bool DHTSensor::begin() {
    dht->begin();
    sensor_t sensor;
    dht->temperature().getSensor(&sensor);
    delayMS = sensor.min_delay / 1000;
    return checkConnection();
}

/**
 * @brief Check if the DHT sensor is properly connected and responding
 * @return true if sensor is connected, false otherwise
 */
bool DHTSensor::checkConnection() {
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    return !isnan(event.temperature);
}

/**
 * @brief Read the current temperature from the DHT sensor
 * @return Temperature in degrees Celsius, or -999.0 if reading failed
 */
float DHTSensor::readTemperature() {
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    return isnan(event.temperature) ? -999.0 : event.temperature;
}

/**
 * @brief Read the current humidity from the DHT sensor
 * @return Relative humidity as a percentage, or -999.0 if reading failed
 */
float DHTSensor::readHumidity() {
    sensors_event_t event;
    dht->humidity().getEvent(&event);
    return isnan(event.relative_humidity) ? -999.0 : event.relative_humidity;
}

/**
 * @brief Print detailed information about the DHT sensor to the log
 * 
 * Displays sensor type, driver version, unique ID, value ranges, and resolution
 * for both temperature and humidity sensors.
 */
void DHTSensor::printSensorDetails() {
    sensor_t sensor;
    
    // Print temperature sensor details
    dht->temperature().getSensor(&sensor);
    SysLogs::println("------------------------------------");
    SysLogs::println("Temperature Sensor");
    SysLogs::print("Sensor Type: "); SysLogs::println(String(sensor.name));
    SysLogs::print("Driver Ver:  "); SysLogs::println(String(sensor.version));
    SysLogs::print("Unique ID:   "); SysLogs::println(String(sensor.sensor_id));
    SysLogs::print("Max Value:   "); SysLogs::print(String(sensor.max_value)); SysLogs::println("°C");
    SysLogs::print("Min Value:   "); SysLogs::print(String(sensor.min_value)); SysLogs::println("°C");
    SysLogs::print("Resolution:  "); SysLogs::print(String(sensor.resolution)); SysLogs::println("°C");
    SysLogs::println("------------------------------------");
    
    // Print humidity sensor details
    dht->humidity().getSensor(&sensor);
    SysLogs::println("Humidity Sensor");
    SysLogs::print("Sensor Type: "); SysLogs::println(String(sensor.name));
    SysLogs::print("Driver Ver:  "); SysLogs::println(String(sensor.version));
    SysLogs::print("Unique ID:   "); SysLogs::println(String(sensor.sensor_id));
    SysLogs::print("Max Value:   "); SysLogs::print(String(sensor.max_value)); SysLogs::println("%");
    SysLogs::print("Min Value:   "); SysLogs::print(String(sensor.min_value)); SysLogs::println("%");
    SysLogs::print("Resolution:  "); SysLogs::print(String(sensor.resolution)); SysLogs::println("%");
    SysLogs::println("------------------------------------");
}
