#include "dataProvider.h"

// Constructor
SensorDataManager::SensorDataManager()
{
    // sensorDataList is automatically constructed
    // any additional initialization can go here
}

// Destructor
SensorDataManager::~SensorDataManager()
{
    // Cleanup code if needed
    resetSensorData();
}

/*****************************************
 * SENSOR DATA MANAGEMENT FUNCTIONS - CRUD
 * ***************************************/

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
        Serial.println("Warning: Invalid sensor data not added");
    }
}

// Get all Sensor Data (improved to return const reference)
const std::vector<sensorData> &SensorDataManager::getAllSensorData() const
{
    return sensorDataList;
}

// Get count of sensor data items
size_t SensorDataManager::getSensorDataCount() const
{
    return sensorDataList.size();
}

// Find sensor by ID
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

// Limit data list size to prevent memory issues
void SensorDataManager::limitDataListSize(size_t maxSize)
{
    if (sensorDataList.size() > maxSize)
    {
        // Remove oldest entries (those at the beginning of the vector)
        sensorDataList.erase(sensorDataList.begin(), sensorDataList.begin() + (sensorDataList.size() - maxSize));
    }
}

// Validate sensor data structure
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

/*****************************************
 * SENSOR DATA JSON Functions
 * ***************************************/

String SensorDataManager::convertSensorDataToJSON(const sensorData &data, String deviceID)
{
    // Create a JSON Document
    JsonDocument sensorDocument;

    // Remove any null characters from deviceID or sensorData fields
    String sanitizedID = removeNullCharacters(deviceID);
    String sanitizedSensorID = removeNullCharacters(data.sensorID);

    // Add device ID
    sensorDocument["ID"] = sanitizedID;

    // Create a JSON Array
    JsonArray sensorDataArray = sensorDocument["data"].to<JsonArray>();

    // Create a JSON Object
    JsonObject sensorData = sensorDataArray.add<JsonObject>();

    // Add the sensor data to the JSON Object
    sensorData["ID"] = sanitizedSensorID;
    sensorData["ST"] = data.status;
    sensorData["TS"] = data.timestamp;

    // Create JSON Arrays for sensor types, units, and values
    JsonArray types = sensorData["TP"].to<JsonArray>();
    JsonArray units = sensorData["UN"].to<JsonArray>();
    JsonArray values = sensorData["VAL"].to<JsonArray>();

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

// Function to remove null characters from a string
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

/*****************************************
 * SENSOR DATA Reseting Functions
 * ***************************************/

void SensorDataManager::resetSensorData()
{
    sensorDataList.clear();
}

/*****************************************
 * DEBUGGING FUNCTIONS
 * ***************************************/

void SensorDataManager::printAllSensorData()
{
    for (const auto &data : sensorDataList)
    {
        Serial.print("Sensor ID: ");
        Serial.println(data.sensorID);
        Serial.print("Sensor Type: ");
        for (const auto &type : data.sensorType)
        {
            Serial.print(type);
            Serial.print(" ");
        }
        Serial.println();
        Serial.print("Status: ");
        Serial.println(data.status);
        Serial.print("Unit: ");
        for (const auto &unit : data.unit)
        {
            Serial.print(unit);
            Serial.print(" ");
        }
        Serial.println();
        Serial.print("Timestamp: ");
        Serial.println(data.timestamp);
        Serial.print("Values: ");
        for (const auto &value : data.values)
        {
            Serial.print(value);
            Serial.print(" ");
        }
        Serial.println();
        Serial.println("----------------------------");
    }
}
