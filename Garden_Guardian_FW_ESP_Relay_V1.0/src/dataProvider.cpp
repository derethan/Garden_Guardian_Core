/**
 * @file dataProvider.cpp
 * @brief Sensor data management and JSON conversion implementation
 * 
 * Manages a collection of sensor data, validates data integrity, and provides
 * JSON serialization for data transmission. Includes size limiting to prevent
 * memory overflow on embedded systems.
 */

#include "dataProvider.h"
#include "base/sysLogs.h"

/**
 * @brief Construct a new Sensor Data Manager object
 * @param cat Category name for this sensor data collection (e.g., "greenhouse")
 */
SensorDataManager::SensorDataManager(String cat) : category(cat)
{
    // sensorDataList is automatically constructed
    // any additional initialization can go here
}

/**
 * @brief Destroy the Sensor Data Manager object and clean up resources
 */
SensorDataManager::~SensorDataManager()
{
    // Cleanup code if needed
    resetSensorData();
}

/**
 * @brief Add a sensor data entry to the collection
 * @param data The sensor data structure to add
 * 
 * Validates the data before adding to ensure data integrity.
 * Automatically limits the collection size to prevent memory issues.
 */
void SensorDataManager::addSensorData(const sensorData &data)
{
    // Validate data before adding
    if (validateSensorData(data))
    {
        sensorDataList.push_back(data);
        // Limit size to prevent memory issues
        limitDataListSize();
    }
    else
    {
        SysLogs::logWarning("Invalid sensor data not added");
    }
}

/**
 * @brief Get all sensor data in the collection
 * @return const reference to the vector of sensor data
 */
const std::vector<sensorData> &SensorDataManager::getAllSensorData() const
{
    return sensorDataList;
}

/**
 * @brief Get the count of sensor data items in the collection
 * @return Number of sensor data entries
 */
size_t SensorDataManager::getSensorDataCount() const
{
    return sensorDataList.size();
}

/**
 * @brief Find a sensor data entry by its ID
 * @param sensorId The sensor ID to search for
 * @param result Output parameter to store the found sensor data
 * @return true if sensor found, false otherwise
 */
bool SensorDataManager::findSensorById(const String &sensorId, sensorData &result) const
{
    for (const auto &data : sensorDataList)
    {
        if (data.sensorID == sensorId)
        {
            result = data;
            return true;
        }
    }
    return false;
}

/**
 * @brief Limit the data list size to prevent memory overflow
 * @param maxSize Maximum number of sensor data entries to keep (default: 100)
 * 
 * Removes oldest entries (FIFO) when the limit is exceeded.
 */
void SensorDataManager::limitDataListSize(size_t maxSize)
{
    if (sensorDataList.size() > maxSize)
    {
        // Remove oldest entries (those at the beginning of the vector)
        sensorDataList.erase(sensorDataList.begin(), sensorDataList.begin() + (sensorDataList.size() - maxSize));
    }
}

/**
 * @brief Validate sensor data structure for integrity
 * @param data The sensor data structure to validate
 * @return true if data is valid, false otherwise
 * 
 * Checks:
 * - Sensor ID is not empty
 * - Sensor type, unit, and values vectors have matching sizes
 * - At least one sensor type is present
 */
bool SensorDataManager::validateSensorData(const sensorData &data)
{
    // Check if sensorID is not empty
    if (data.sensorID.isEmpty())
    {
        return false;
    }

    // Check that types, units, and values vectors have the same size
    if (data.sensorType.size() != data.unit.size() ||
        data.sensorType.size() != data.values.size())
    {
        return false;
    }

    // Check that we have at least one sensor type
    if (data.sensorType.empty())
    {
        return false;
    }

    return true;
}

/**
 * @brief Convert sensor data to JSON format
 * @param data The sensor data to convert
 * @param deviceID The device identifier to include in the JSON
 * @return JSON string representation of the sensor data
 * 
 * Creates a JSON document with device ID, category, and sensor data arrays
 * including sensor types, units, and values.
 */
String SensorDataManager::convertSensorDataToJSON(const sensorData &data, String deviceID)
{
    // Create a JSON Document
    JsonDocument sensorDocument;

    // Remove any null characters from deviceID or sensorData fields
    String sanitizedID = removeNullCharacters(deviceID);
    String sanitizedSensorID = removeNullCharacters(data.sensorID);

    // Add device ID
    sensorDocument["ID"] = sanitizedID;
    sensorDocument["category"] = category;

    // Create a JSON Array
    JsonArray sensorDataArray = sensorDocument["data"].to<JsonArray>();

    // Create a JSON Object
    JsonObject sensorDataObj = sensorDataArray.add<JsonObject>();

    // Add the sensor data to the JSON Object
    sensorDataObj["ID"] = sanitizedSensorID;
    sensorDataObj["ST"] = data.status;
    sensorDataObj["TS"] = data.timestamp;
    sensorDataObj["SN"] = removeNullCharacters(data.sensorName);

    // Create JSON Arrays for sensor types, units, and values
    JsonArray types = sensorDataObj["TP"].to<JsonArray>();
    JsonArray units = sensorDataObj["UN"].to<JsonArray>();
    JsonArray values = sensorDataObj["VAL"].to<JsonArray>();

    // Populate arrays
    for (auto &type : data.sensorType)
    {
        types.add(type);
    }
    for (auto &unit : data.unit)
    {
        units.add(unit);
    }
    for (auto &val : data.values)
    {
        values.add(val);
    }

    // Serialize the JSON Document to a String
    String jsonString;
    serializeJson(sensorDocument, jsonString);

    // Return the JSON String
    return jsonString;
}

/**
 * @brief Remove null characters from a string
 * @param input The input string to sanitize
 * @return Sanitized string without null characters
 * 
 * Prevents null character injection issues in JSON strings.
 */
String SensorDataManager::removeNullCharacters(const String &input)
{
    String output;
    for (char c : input)
    {
        if (c != '\0')
        {
            output += c;
        }
    }
    return output;
}

/**
 * @brief Reset/clear all sensor data from the collection
 * 
 * Removes all sensor data entries, typically called after successful
 * data transmission to free memory.
 */
void SensorDataManager::resetSensorData()
{
    sensorDataList.clear();
}

/**
 * @brief Print all sensor data to the debug log
 * 
 * Displays detailed information about each sensor data entry including
 * sensor ID, types, status, units, timestamps, and values.
 * Only outputs when DEBUG_MODE is enabled.
 */
void SensorDataManager::printAllSensorData()
{
    for (const auto &data : sensorDataList)
    {
        SysLogs::print("Sensor ID: ");
        SysLogs::println(data.sensorID);
        SysLogs::print("Sensor Type: ");
        for (const auto &type : data.sensorType)
        {
            SysLogs::print(type);
            SysLogs::print(" ");
        }
        SysLogs::println();
        SysLogs::print("Status: ");
        SysLogs::println(String(data.status));
        SysLogs::print("Unit: ");
        for (const auto &unit : data.unit)
        {
            SysLogs::print(unit);
            SysLogs::print(" ");
        }
        SysLogs::println();
        SysLogs::print("Timestamp: ");
        SysLogs::println(String(data.timestamp));
        SysLogs::print("Values: ");
        for (const auto &value : data.values)
        {
            SysLogs::print(String(value));
            SysLogs::print(" ");
        }
        SysLogs::println();
        SysLogs::println("----------------------------");
    }
}
