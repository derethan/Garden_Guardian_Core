#include "tempSensors.h"

TempSensors::TempSensors()
    : oneWire(TEMP_SENSOR_PIN), sensors(&oneWire), sensorCount(0)
{
    for (uint8_t i = 0; i < MAX_DS18B20_SENSORS; ++i) {
        sensorConnected[i] = false;
        lastTemp[i] = DEVICE_DISCONNECTED_C;
    }
}

void TempSensors::initialize()
{
    sensors.begin();
    delay(1000); // Allow time for sensors to initialize
    sensorCount = sensors.getDeviceCount();
    for (uint8_t i = 0; i < sensorCount && i < MAX_DS18B20_SENSORS; ++i) {
        if (sensors.getAddress(sensorAddresses[i], i)) {
            sensorConnected[i] = true;
        } else {
            sensorConnected[i] = false;
        }
    }
    sensors.requestTemperatures();
    delay(1000); // Delay for the sensor to initialize
    if (DEBUG_MODE)
    {
        Serial.println("Temperature Sensors Initialized");
        Serial.print("Sensors Detected: ");
        Serial.println(sensorCount);
        for (uint8_t i = 0; i < sensorCount; ++i) {
            Serial.print("Sensor "); Serial.print(i); Serial.print(" Connected: ");
            Serial.println(sensorConnected[i] ? "Yes" : "No");
        }
    }
}

float TempSensors::readSensor(uint8_t index)
{
    if (index >= sensorCount || !sensorConnected[index])
        return DEVICE_DISCONNECTED_C;
    const int NUM_READS = 5;
    float sum = 0;
    int validReads = 0;
    for (int i = 0; i < NUM_READS; i++)
    {
        sensors.requestTemperaturesByAddress(sensorAddresses[index]);
        float temp = sensors.getTempC(sensorAddresses[index]);
        if (temp != DEVICE_DISCONNECTED_C)
        {
            sum += temp;
            validReads++;
        }
        delay(100);
    }
    float result = (validReads > 0) ? (sum / validReads) : DEVICE_DISCONNECTED_C;
    lastTemp[index] = result;
    sensorConnected[index] = (result != DEVICE_DISCONNECTED_C);
    if (DEBUG_MODE)
    {
        Serial.print("Sensor "); Serial.print(index); Serial.print(" Temperature: ");
        Serial.println(result);
    }
    return result;
}

int TempSensors::readAllSensors(float *temps, int maxCount)
{
    int count = (sensorCount < maxCount) ? sensorCount : maxCount;
    sensors.requestTemperatures();
    for (int i = 0; i < count; ++i) {
        if (sensorConnected[i]) {
            temps[i] = sensors.getTempC(sensorAddresses[i]);
            lastTemp[i] = temps[i];
        } else {
            temps[i] = DEVICE_DISCONNECTED_C;
        }
    }
    return count;
}

uint8_t TempSensors::getSensorCount() const
{
    return sensorCount;
}

bool TempSensors::isSensorConnected(uint8_t index) const
{
    if (index >= sensorCount) return false;
    return sensorConnected[index];
}

bool TempSensors::getSensorAddress(uint8_t index, DeviceAddress address) const
{
    if (index >= sensorCount) return false;
    memcpy(address, sensorAddresses[index], sizeof(DeviceAddress));
    return true;
}
