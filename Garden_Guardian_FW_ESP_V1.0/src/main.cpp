#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "esp_task_wdt.h"

#include "secrets.h"
#include "config.h"

// Module Imports
#include "networkConnections.h"
#include "getTime.h"
#include "relayControl.h"
#include "dataProvider.h"
#include "tempSensors.h"

/*****************************************
 * Global State Variables
 *****************************************/
enum class SystemMode
{
  INITIALIZING,
  NORMAL_OPERATION,
  ERROR,
  CONFIG_MODE
};

struct SystemState
{
  unsigned long lastSensorRead = 0;
  unsigned long lastCollectionTime = 0;
  unsigned long lastPublishTime = 0;
  unsigned long currentTime = 0;
  unsigned long lastWiFiRetry = 0;
  uint32_t lastTimeSyncEpoch = 0;
  bool wasConnected = false;
  String deviceID;
  String idCode;
  SystemMode currentMode = SystemMode::INITIALIZING;
  bool sensorError = false;
  uint32_t lastErrorTime = 0;
  String lastErrorMessage;
  bool apAlwaysOn = false;
  uint32_t collection_interval = 30; // 30 seconds
  unsigned long relayReadInterval = 60;
  unsigned long lastRelayRead = 0;
  unsigned long relayScheduleOnHour = 16;
  unsigned long RelayScheduleOffHour = 10;
  unsigned long relayTimerOnInterval = 0;
  unsigned long relayTimerOffInterval = 0;
  float DWS_Res_Temp = 0;
  float Target_DWS_Res_Temp = 18;
  float NFT_Res_Temp = 0;
  float Target_NFT_Res_Temp = 18;
  float Current_Air_Temp = 0;
  float Target_Air_Temp = 25;
} state;

// BLE State structure
struct BLEState
{
  BLEClient *client = nullptr;
  BLERemoteService *remoteService = nullptr;
  BLERemoteCharacteristic *tempCharacteristic = nullptr;
  BLERemoteCharacteristic *humidityCharacteristic = nullptr;
  BLEScan *scanner = nullptr;
} bleState;

/*****************************************
 * Global Objects
 *****************************************/
NetworkConnections network;
TimeRetriever timeRetriever;
SensorDataManager sensorData;

// Relay Objects
RelayControl relay1(RELAY1_PIN);
RelayControl relay2(RELAY2_PIN);

RelayControl relay3(RELAY3_PIN, 5.0);
RelayControl relay4(RELAY4_PIN, 5.0);

RelayControl relays[] = {relay1, relay2, relay3, relay4};

// Temperature Sensors
TempSensors tempSensors;

/*****************************************
 * Forward Declarations
 *****************************************/
bool initializeBLE();
bool scanAndConnectDevice();
bool setupSensorCharacteristics();
bool readBTSensorData();
void readSensorData();
void setupNetwork();
bool readTempData();
bool readTDSData();

/*****************************************
 * Utility Functions
 *****************************************/
void logError(const char *message)
{
  state.lastErrorMessage = message;
  state.lastErrorTime = state.currentTime;
  if (DEBUG_MODE)
  {
    Serial.print("ERROR: ");
    Serial.println(message);
  }
}

/*****************************************
 * Bluetooth Functions
 *****************************************/

bool initializeBLE()
{
  BLEDevice::init("");
  bleState.scanner = BLEDevice::getScan();

  if (bleState.scanner != nullptr)
  {
    bleState.scanner->setActiveScan(true);
    bleState.scanner->setInterval(1349);
    bleState.scanner->setWindow(449);
    Serial.println("BLE Scanner initialized successfully");
    return true;
  }
  Serial.println("Failed to initialize BLE Scanner");
  return false;
}

bool setupSensorCharacteristics()
{
  bleState.remoteService = bleState.client->getService(BLEUUID(SERVICE_UUID));
  if (bleState.remoteService == nullptr)
  {
    Serial.println("Failed to find service");
    return false;
  }

  bleState.tempCharacteristic = bleState.remoteService->getCharacteristic(BLEUUID(TEMP_CHAR_UUID));
  bleState.humidityCharacteristic = bleState.remoteService->getCharacteristic(BLEUUID(HUMIDITY_CHAR_UUID));

  if (bleState.tempCharacteristic == nullptr || bleState.humidityCharacteristic == nullptr)
  {
    Serial.println("Failed to find characteristics");
    return false;
  }

  return true;
}

bool scanAndConnectDevice()
{
  if (bleState.scanner == nullptr)
  {
    Serial.println("Error: BLE Scanner not initialized");
    return false;
  }

  try
  {
    bleState.client = BLEDevice::createClient();
    delay(1000);

    BLEScanResults scanResults = bleState.scanner->start(3, false);
    Serial.printf("Scan complete. Found %d devices\n", scanResults.getCount());
    yield();

    for (int i = 0; i < scanResults.getCount(); i++)
    {
      esp_task_wdt_reset();
      BLEAdvertisedDevice device = scanResults.getDevice(i);

      if (device.getName() == "GG-ENV")
      {
        if (!bleState.client->connect(&device))
        {
          Serial.println("Connection failed");
          continue;
        }

        Serial.println("Connected to device");
        if (setupSensorCharacteristics())
        {
          Serial.println("Connected to GG-ENV successfully");
          bleState.scanner->clearResults();
          return true;
        }

        bleState.client->disconnect();
      }
      yield();
      delay(100);
    }
    bleState.scanner->clearResults();
  }
  catch (const std::exception &e)
  {
    Serial.println("Error during BLE operations");
    if (bleState.client != nullptr)
    {
      delete bleState.client;
      bleState.client = nullptr;
    }
  }
  return false;
}

// ToDo: This function will need to be adapted to a general read from BT Devices function
bool readBTSensorData()
{
  esp_task_wdt_reset();

  if (bleState.client == nullptr || !bleState.client->isConnected() ||
      bleState.tempCharacteristic == nullptr || bleState.humidityCharacteristic == nullptr)
  {
    Serial.println("No Device connected");
    if (bleState.client != nullptr)
    {
      bleState.client->disconnect();
      delete bleState.client;
      bleState.client = nullptr;
    }
    return false;
  }

  if (bleState.tempCharacteristic->canRead())
  {
    std::string tempRawValue = bleState.tempCharacteristic->readValue();
    std::string humRawValue = bleState.humidityCharacteristic->readValue();

    if (tempRawValue.length() > 0 && humRawValue.length() > 0)
    {
      float temperature = atof(tempRawValue.c_str());
      float humidity = atof(humRawValue.c_str());

      if (temperature < -40 || temperature > 80 || humidity < 0 || humidity > 100)
      {
        Serial.println("Invalid data received");
        return false;
      }

      // Add sensor data to the list, first create a sensorData object
      sensorData.addSensorData({.sensorID = "GG-TH1",
                                .sensorType = {"Temperature", "Humidity"},
                                .status = 0,
                                .unit = {"Celsius", "Percentage"},
                                .timestamp = state.currentTime,
                                .values = {temperature, humidity}});

      return true;
    }
    Serial.println("No data received from sensor");
    return false;
  }
  Serial.println("Cannot read characteristics");
  return false;
}

void setupNetwork()
{
  WiFiCredentials credentials = network.loadWiFiCredentials();

  network.setupWiFi(credentials, state.idCode, state.apAlwaysOn);

  if (network.isConnected())
  {
    timeRetriever.initialize();
    network.getTime();
  }
}

/*****************************************
 * Sensor Functions
 *****************************************/
void readSensorData()
{
  if (!readTempData())
  {
    logError("Sensor Error: Failed to read temperature data");
    state.sensorError = true;
  }
  else
  {
    state.sensorError = false;
  }

  if (!readTDSData())
  {
    logError("Sensor Error: Failed to read TDS data");
    state.sensorError = true;
  }
  else
  {
    state.sensorError = false;
  }
}

bool readTempData()
{
  state.DWS_Res_Temp = tempSensors.readSensor1();     // DWC_Res_Temp
  state.NFT_Res_Temp = tempSensors.readSensor2();     // NFT_Res_Temp
  state.Current_Air_Temp = tempSensors.readSensor3(); // Current_Air_Temp

  if (state.DWS_Res_Temp == -127.0 || state.NFT_Res_Temp == -127.0 || state.Current_Air_Temp == -127.0)
  {
    logError("Sensor Error: Invalid temperature reading");
    return false;
  }

  sensorData.addSensorData({.sensorID = "DS18B20-1",
                            .sensorType = {"Temperature"},
                            .status = 0,
                            .unit = {"Celsius"},
                            .timestamp = state.currentTime,
                            .values = {state.DWS_Res_Temp}});
  sensorData.addSensorData({.sensorID = "DS18B20-2",
                            .sensorType = {"Temperature"},
                            .status = 0,
                            .unit = {"Celsius"},
                            .timestamp = state.currentTime,
                            .values = {state.NFT_Res_Temp}});
  sensorData.addSensorData({.sensorID = "DS18B20-3",
                            .sensorType = {"Temperature"},
                            .status = 0,
                            .unit = {"Celsius"},
                            .timestamp = state.currentTime,
                            .values = {state.Current_Air_Temp}});
  return true;
}

bool readTDSData()
{
  // Placeholder for TDS sensor reading logic
  // Implement the actual TDS reading logic here

  // For now, we'll just simulate a successful read with a dummy value
  float tdsValue = analogRead(TDS_SENSOR_PIN); // Replace with actual TDS reading logic
  sensorData.addSensorData({.sensorID = "TDS-1",
                            .sensorType = {"TDS"},
                            .status = 0,
                            .unit = {"PPM"},
                            .timestamp = state.currentTime,
                            .values = {tdsValue}});
  if (tdsValue < 0 || tdsValue > 10000) // Example range check for TDS values
  {
    logError("Sensor Error: Invalid TDS reading");
    return false;
  }
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(2000); // Delay for the serial monitor to start

  // Welcome Message
  Serial.println("Garden Guardian - Environmental System");

  // Configure watchdog
  esp_task_wdt_init(10, true); // 10 second timeout, panic on timeout
  esp_task_wdt_add(NULL);      // Add current task to WDT watch

  // Initialize BLE Module
  if (!initializeBLE())
  {
    state.currentMode = SystemMode::ERROR;
    return;
  }
  Serial.println("BLE Initialized");

  // Initialize Temperature Sensors
  tempSensors.initialize();

  // Initialize Network
  setupNetwork();
}

void loop()
{
  esp_task_wdt_reset();

  // Update system mode based on conditions
  switch (state.currentMode)
  {
  case SystemMode::INITIALIZING:
    if (network.isConnected())
    {
      state.currentMode = SystemMode::NORMAL_OPERATION;

      // Initialize system state: Time
      state.currentTime = network.getRTCTime();
      state.lastTimeSyncEpoch = network.getRTCTime();
      state.lastSensorRead = network.getRTCTime();
    }
    else if (network.isAPMode())
    {
      state.currentMode = SystemMode::CONFIG_MODE;
      Serial.println("Entering Configuration Mode, Awaiting Network Configuration...");
      state.currentTime = millis();
    }
    break;

  case SystemMode::NORMAL_OPERATION:

    // Update current time
    if (network.isConnected())
    {
      state.currentTime = network.getRTCTime();
    }
    else
    {
      state.currentTime = millis();
    }

    // Check if BLE connection is active, if not, try to connect
    if (bleState.client == nullptr)
    {
      if (!scanAndConnectDevice())
      {
        delay(1000);
        return;
      }
    }

    // Debug time tracking parameters
    if (DEBUG_MODE && (state.currentTime % 10 == 0))
    { // Print every ~10 seconds
      Serial.println("\n----- TIME TRACKING DEBUG -----");
      Serial.print("Current Time: ");
      Serial.print(state.currentTime);
      if (network.isConnected())
      {
        Serial.println(" (RTC)");
      }
      else
      {
        Serial.println(" (millis)");
      }

      // Calculate time differences using uint32_t to handle large values
      uint32_t timeSinceLastRead = (uint32_t)(state.currentTime - state.lastSensorRead);
      uint32_t timeUntilNextRead = (uint32_t)(state.collection_interval - timeSinceLastRead);
      uint32_t timeSinceSync = (uint32_t)(state.currentTime - state.lastTimeSyncEpoch);

      Serial.print("Last Sensor Read: ");
      Serial.print(state.lastSensorRead);
      Serial.print(" (");
      Serial.print(timeSinceLastRead);
      Serial.println(" seconds ago)");

      Serial.print("Next Sensor Read in: ");
      Serial.print(timeUntilNextRead);
      Serial.println(" seconds");

      Serial.print("Last Time Sync: ");
      if (state.lastTimeSyncEpoch > 0)
      {
        Serial.print(state.lastTimeSyncEpoch);
        Serial.print(" (");
        Serial.print((state.currentTime - state.lastTimeSyncEpoch));
        Serial.println(" seconds ago)");
      }
      else
      {
        Serial.println("Never");
      }

      Serial.print("Collection Interval: ");
      Serial.print(state.collection_interval);
      Serial.println(" seconds");
      Serial.println("-----------------------------");
    }

    // Check if it's time to read Data from connected Sensors
    if ((uint32_t)(state.currentTime - state.lastSensorRead) >= state.collection_interval)
    {
      Serial.println("Reading sensor data..."); // Add this debug line

      readSensorData();

      // Read Data from connected Bluetooth Devices
      if (readBTSensorData())
      {
        Serial.println("Sensor data read successfully");
      }
      else
      {
        Serial.println("Failed to read sensor data");
      }
      state.lastSensorRead = state.currentTime;
    }

    // Set Relays based on schedule or conditions
    if (state.currentTime - state.lastRelayRead >= state.relayReadInterval)
    {
      // Message
      Serial.println("Checking Relays");

      relay1.setRelayForSchedule(state.relayScheduleOnHour, state.RelayScheduleOffHour, timeRetriever.getCurrentTime());
      relay2.setRelayforTemp(state.Current_Air_Temp, state.Target_Air_Temp); // Current/Target
      relay3.setRelayforTemp(state.DWS_Res_Temp, state.Target_DWS_Res_Temp); // Current/Target
      relay4.setRelayforTemp(state.NFT_Res_Temp, state.Target_NFT_Res_Temp); // Current/Target

      state.lastRelayRead = state.currentTime;
    }

    Serial.print(".");

    break;

    // System Mode: Error
    // This mode is entered when a sensor error is detected
  case SystemMode::ERROR:
    break;

    // System Mode: Configuration
    // This mode is entered when the system is in AP mode or Web Server mode
  case SystemMode::CONFIG_MODE:
    network.processDNSRequests(); // Process DNS first
    network.handleClientRequests();
    break;
  }

  yield();
  delay(1000);
}
