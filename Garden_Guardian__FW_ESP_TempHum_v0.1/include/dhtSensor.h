#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

class DHTSensor {
private:
    DHT_Unified* dht;
    uint8_t pin;
    uint8_t type;
    uint32_t delayMS;

public:
    DHTSensor(uint8_t pin, uint8_t type);
    ~DHTSensor();
    
    bool begin();
    bool checkConnection();
    float readTemperature();
    float readHumidity();
    void printSensorDetails();
    uint32_t getMinDelay() const { return delayMS; }
};

#endif // DHT_SENSOR_H
