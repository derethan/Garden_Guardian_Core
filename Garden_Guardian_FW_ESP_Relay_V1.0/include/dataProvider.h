#ifndef DATAPROVIDER_H
#define DATAPROVIDER_H

#include <Arduino.h>
#include "ArduinoJson.h"
#include <vector>
#include <map>
#include "Config.h"

struct sensorData
{
    String sensorID;                // Unique identifier for the sensor
    std::vector<String> sensorType; // Array of sensor types (e.g., Temp, NPK)
    String sensorName;              // Type of sensor in Unit (DHT, DS18B20, etc.)
    int status;                     // 200 = OK, 500 = ERROR, etc.
    std::vector<String> unit;       // Array of units of measurement
    unsigned long timestamp;        // Timestamp of the reading
    std::vector<float> values;      // Vector to store multiple values
};

// Class to encapsulate sensor data management
class SensorDataManager
{
private:
    String category;
    // Vector to store sensor data
    std::vector<sensorData> sensorDataList;

    // Sanitize input string
    String removeNullCharacters(const String &input);

    // Validate sensor data structure
    bool validateSensorData(const sensorData &data);

public:
    // Constructor
    SensorDataManager(String cat);

    // Destructor
    ~SensorDataManager();

    // Add sensor data to the vector
    void addSensorData(const sensorData &data);

    // Reset sensor data
    void resetSensorData();

    // Convert sensor data to JSON format
    String convertSensorDataToJSON(const sensorData &data, String deviceID);

    // Print all sensor data
    void printAllSensorData();

    // Get all sensor data
    const std::vector<sensorData> &getAllSensorData() const;

    // Get count of sensor data items
    size_t getSensorDataCount() const;

    // Find sensor by ID
    bool findSensorById(const String &sensorId, sensorData &result) const;

    // Limit data list size to prevent memory issues
    void limitDataListSize(size_t maxSize = 100);

    // Singleton pattern (optional)
    static SensorDataManager &getInstance(String cat = "greenhouse")
    {
        static SensorDataManager instance(cat);
        return instance;
    }
};

#endif // DATAPROVIDER_H