#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "esp_task_wdt.h"

#include "secrets.h"
#include "config.h"
#include "state.h"

// Module Imports
#include "networkConnections.h"
#include "getTime.h"
#include "relayControl.h"
#include "dataProvider.h"
#include "tempSensors.h"
#include "tdsSensor.h"


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
RelayControl relay2(RELAY2_PIN, 3.0);

RelayControl relay3(RELAY3_PIN, 5.0);
RelayControl relay4(RELAY4_PIN, 5.0);

// Auto Control Unit Relays (Food, Water, PH)
RelayControl TDSController(TDS_CTRL_PIN, state.tdsHysteresis); // TDS Controller Relay

// Temperature Sensors
TempSensors tempSensors;

// TDS Sensor
TDSSensor tdsSensor(TDS_SENSOR_PIN, VREF, SCOUNT);

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
 * Configuration Functions
 *****************************************/
void loadDeviceSettings()
{
  Serial.println("[SYSTEM] Loading device settings...");

  DeviceSettings settings = network.loadDeviceSettings();

  if (settings.valid)
  { // Apply loaded settings
    state.SLEEP_DURATION = settings.sleepDuration;
    state.sensorRead_interval = settings.sensorReadInterval;
    state.SENSOR_STABILIZATION_TIME = settings.sensorStabilizationTime;
    state.deviceID = settings.deviceID;
    state.idCode = settings.idCode;
    state.httpPublishEnabled = settings.httpPublishEnabled;
    state.httpPublishInterval = settings.httpPublishInterval;

    Serial.println("[SYSTEM] Device settings applied:");
    Serial.print("  Sleep Duration: ");
    Serial.print(state.SLEEP_DURATION / 1000000ULL);
    Serial.println(" seconds");
    Serial.print("  Sensor Read Interval: ");
    Serial.print(state.sensorRead_interval / 1000);
    Serial.println(" seconds");
    Serial.print("  Stabilization Time: ");
    Serial.print(state.SENSOR_STABILIZATION_TIME / 1000);
    Serial.println(" seconds");
    Serial.print("  Device ID: ");
    Serial.println(state.deviceID);
    Serial.print("  ID Code: ");
    Serial.println(state.idCode);
    Serial.print("  HTTP Publishing: ");
    Serial.println(state.httpPublishEnabled ? "Enabled" : "Disabled");
    if (state.httpPublishEnabled)
    {
      Serial.print("  HTTP Publish Interval: ");
      Serial.print(state.httpPublishInterval / 1000);
      Serial.println(" seconds");
    }
  }
  else
  {
    Serial.println("[SYSTEM] Using default device settings");
  }
}

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
    // Serial.printf("Scan complete. Found %d devices\n", scanResults.getCount());
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
    Serial.println("Error: BLE connection not established");
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
  uint8_t sensorCount = tempSensors.getSensorCount();
  bool allValid = true;
  float temps[MAX_DS18B20_SENSORS] = {DEVICE_DISCONNECTED_C};

  // Read all sensors
  for (uint8_t i = 0; i < sensorCount; ++i)
  {
    temps[i] = tempSensors.readSensor(i);
    if (temps[i] == DEVICE_DISCONNECTED_C)
    {
      allValid = false;
    }
  }

  // Update state variables for first two sensors (for compatibility)
  if (sensorCount > 0)
    state.DWC_Res_Temp = temps[0];
  if (sensorCount > 1)
    state.NFT_Res_Temp = temps[1];

  // Add each sensor's data to sensorData
  for (uint8_t i = 0; i < sensorCount; ++i)
  {
    char sensorId[16];
    snprintf(sensorId, sizeof(sensorId), "DS18B20-%u", i + 1);
    sensorData.addSensorData({.sensorID = sensorId,
                              .sensorType = {"Temperature"},
                              .status = (temps[i] == DEVICE_DISCONNECTED_C) ? 1 : 0,
                              .unit = {"Celsius"},
                              .timestamp = state.currentTime,
                              .values = {temps[i]}});
  }

  // Optionally, handle error if any sensor is disconnected
  if (!allValid)
  {
    logError("One or more DS18B20 sensors not connected or failed to read");
    return false;
  }
  return true;
}

bool readTDSData()
{
  state.tdsValue = tdsSensor.read(state.NFT_Res_Temp); // Use NFT_Res_Temp for temperature compensation

  sensorData.addSensorData({.sensorID = "TDS-1",
                            .sensorType = {"TDS"},
                            .status = 0,
                            .unit = {"PPM"},
                            .timestamp = state.currentTime,
                            .values = {state.tdsValue}});

  // Check if the TDS value is within a reasonable range
  if (state.tdsValue < 0 || state.tdsValue > 10000) // Example range check for TDS values
  {
    logError("Sensor Error from readTDSData (): Invalid TDS reading");
    return false;
  }
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(5000); // Delay for the serial monitor to start

  // Welcome Message
  Serial.println("Garden Guardian - Environmental System");

  // Configure watchdog
  esp_task_wdt_init(60, true); // 60 second timeout, panic on timeout
  esp_task_wdt_add(NULL);      // Add current task to WDT watch

  // Initialize Temperature Sensors
  tempSensors.initialize();

  // Initialize Relays
  // for (RelayControl &relay : relays)
  // {
  //   relay.initialize();
  // }
  relay1.initialize();
  relay2.initialize();
  relay3.initialize();
  relay4.initialize();

  TDSController.initialize(); // Initialize TDS Controller Relay

  // // Initialize BLE Module
  // if (!initializeBLE())
  // {
  //   state.currentMode = SystemMode::ERROR;
  //   return;
  // }
  // Serial.println("BLE Initialized");

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
      state.lastTimeSyncEpoch = state.currentTime;
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
    // if (bleState.client == nullptr)
    // {
    //   Serial.println("----- BLE Reconnection Status -----");
    //   if (!scanAndConnectDevice())
    //   {
    //     delay(100);

    //     Serial.println("Failed to reconnect to BLE device");
    //     Serial.println("---------------------------------");
    //     return;
    //   }
    //   Serial.println("BLE Reconnection Established");
    //   delay(100);
    //   Serial.println("---------------------------------");
    // }

    // Debug time tracking parameters
    if (DEBUG_MODE && (state.currentTime % 30 == 0)) // Print every ~10 seconds
    {
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
      uint32_t timeUntilNextRead = (uint32_t)(state.sensorRead_interval - timeSinceLastRead);
      uint32_t timeSinceSync = (uint32_t)(state.currentTime - state.lastTimeSyncEpoch);
      uint32_t timeUntilNextSync = (uint32_t)(state.sensorRead_interval - timeSinceSync);
      uint32_t timeSinceLastRelayRead = (uint32_t)(state.currentTime - state.lastRelayRead);
      uint32_t timeUntilNextRelayRead = (uint32_t)(state.relayReadInterval - timeSinceLastRelayRead);

      Serial.print("Last Sensor Read: ");
      Serial.print(state.lastSensorRead);
      Serial.print(" (");
      Serial.print(timeSinceLastRead);
      Serial.println(" seconds ago)");

      Serial.print("Next Sensor Read in: ");
      Serial.print(timeUntilNextRead);
      Serial.println(" seconds");

      // last and next time sync interval for time syncing from network
      Serial.print("Last Time Sync: "); // log last time sync interval for time syncing from network
      Serial.print(state.lastTimeSyncEpoch);
      Serial.print(" (");
      Serial.print(timeSinceSync); // log last time sync interval for time syncing from network
      Serial.println(" seconds ago)");

      // log next time sync
      Serial.print("Next Time Sync in: ");
      Serial.print(timeUntilNextSync);
      Serial.println(" seconds");

      Serial.print("Last Relay Read: ");
      Serial.print(state.lastRelayRead);
      Serial.print(" (");
      Serial.print(timeSinceLastRelayRead); // log last relay read time interval
      Serial.println(" seconds ago)");
      Serial.print("Next Relay Read in: ");
      Serial.print(timeUntilNextRelayRead); // log next relay read time interval
      Serial.println(" seconds");

      Serial.println("-----------------------------");
    }

    // Check if it's time to read Data from connected Sensors
    if (state.lastSensorRead == 0 || state.currentTime - state.lastSensorRead >= state.sensorRead_interval)
    {
      Serial.println("Reading Data from Device Sensors..."); // Add this debug line

      readSensorData();

      if (DEBUG_MODE)
      {
        // print all sensor data
        sensorData.printAllSensorData();

        // Clear the sensor data list after reading
        sensorData.resetSensorData();
      }

      // Read Data from connected Bluetooth Devices
      // if (readBTSensorData())
      // {
      //   Serial.println("BLE Device connected: Data from BLE Device read successfully");
      // }
      // else
      // {
      //   Serial.println("No BLE Device Connected: Skipping Data Read");
      // }
      state.lastSensorRead = state.currentTime;
    }

    if (state.currentTime - state.lastTDSControllerRead >= state.tdsControllerInterval)
    {
      TDSController.setAutoFeedingSystem(state.tdsValue, state.targetTDS, 300000);
      state.lastTDSControllerRead = state.currentTime;
    }

    // Set Relays based on schedule or conditions
    if (state.currentTime - state.lastRelayRead >= state.relayReadInterval)
    {
      // Message
      Serial.println("Checking Relays");

      relay1.setRelayForSchedule(state.relayScheduleOnHour, state.RelayScheduleOffHour, timeRetriever.getCurrentTime());
      relay2.setRelayforTemp(state.Current_Air_Temp, state.Target_Air_Temp); // Current/Target
      relay3.setRelayforTemp(state.DWC_Res_Temp, state.Target_DWC_Res_Temp); // Current/Target
      relay4.setRelayforTemp(state.NFT_Res_Temp, state.Target_NFT_Res_Temp); // Current/Target

      state.lastRelayRead = state.currentTime;

      if (DEBUG_MODE)
      {
        Serial.println("Relay 1: " + String(relay1.isOn() ? "On" : "Off"));
        Serial.println("Relay 2: " + String(relay2.isOn() ? "On" : "Off"));
        Serial.println("Relay 3: " + String(relay3.isOn() ? "On" : "Off"));
        Serial.println("Relay 4: " + String(relay4.isOn() ? "On" : "Off"));
      }
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
    // network.processDNSRequests(); // Process DNS first
    network.handleClientRequests();
    break;
  }

  yield();
  delay(1000);
}
