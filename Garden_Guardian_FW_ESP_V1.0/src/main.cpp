#include <Arduino.h>
#include "esp_task_wdt.h"

#include "secrets.h"
#include "config.h"
#include "state.h"
#include "latestReadings.h"

// Global state variable definition
SystemState state;

// Module Imports
#include "networkConnections.h"
#include "getTime.h"
#include "relayControl.h"
#include "dataProvider.h"
#include "tempSensors.h"
#include "tdsSensor.h"

/*****************************************
 * Global Objects
 *****************************************/
NetworkConnections network;
TimeRetriever timeRetriever;
SensorDataManager sensorData;
LatestReadings latestReadings; // Store latest readings for web display

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
void readSensorData();
void setupNetwork();
// bool readTempData();
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

    // Apply target values
    state.targetTDS = settings.targetTDS;
    state.Target_Air_Temp = settings.targetAirTemp;
    state.Target_NFT_Res_Temp = settings.targetNFTResTemp;
    state.Target_DWC_Res_Temp = settings.targetDWCResTemp;

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
    Serial.print("  Target TDS: ");
    Serial.print(state.targetTDS);
    Serial.println(" ppm");
    Serial.print("  Target Air Temperature: ");
    Serial.print(state.Target_Air_Temp);
    Serial.println("°C");
    Serial.print("  Target NFT Reservoir Temperature: ");
    Serial.print(state.Target_NFT_Res_Temp);
    Serial.println("°C");
    Serial.print("  Target DWC Reservoir Temperature: ");
    Serial.print(state.Target_DWC_Res_Temp);
    Serial.println("°C");
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

void setupNetwork()
{
  WiFiCredentials credentials = network.loadWiFiCredentials();
  network.setupWiFi(credentials, state.idCode, state.apAlwaysOn);

  if (network.isConnected())
  {
    // Attempt NTP synchronization
    unsigned long ntpTime = network.getTime();
    if (ntpTime == 0)
    {
      Serial.println("NTP synchronization failed. Continuing with RTC time if available.");
      // Try to get RTC time as fallback
      unsigned long rtcTime = network.getRTCTime();
      if (rtcTime == 0)
      {
        Serial.println("WARNING: No valid time source available (neither NTP nor RTC)");
        Serial.println("Timestamps in sensor data may be inaccurate");
      }
      else
      {
        Serial.println("Using RTC time as fallback");
      }
    }
    else
    {
      Serial.println("System time synchronized successfully via NTP");
    }

    // Start the web server when connected to WiFi
    network.startWebServer();
  }
}

/*****************************************
 * Sensor Functions
 *****************************************/
void readSensorData()
{
  // if (!readTempData())
  // {
  //   logError("Sensor Error: Failed to read temperature data");
  //   state.sensorError = true;
  // }
  // else
  // {
  //   state.sensorError = false;
  // }

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

// bool readTempData()
// {
//   uint8_t sensorCount = tempSensors.getSensorCount();
//   bool allValid = true;
//   float temps[MAX_DS18B20_SENSORS] = {DEVICE_DISCONNECTED_C};

//   // Read all sensors
//   for (uint8_t i = 0; i < sensorCount; ++i)
//   {
//     temps[i] = tempSensors.readSensor(i);
//     if (temps[i] == DEVICE_DISCONNECTED_C)
//     {
//       allValid = false;
//     }
//   }

//   // Update state variables for first two sensors (for compatibility)
//   if (sensorCount > 0)
//     state.DWC_Res_Temp = temps[0];
//   if (sensorCount > 1)
//     state.NFT_Res_Temp = temps[1];

//   // Add each sensor's data to sensorData
//   for (uint8_t i = 0; i < sensorCount; ++i)
//   {
//     char sensorId[16];
//     snprintf(sensorId, sizeof(sensorId), "DS18B20-%u", i + 1);
//     sensorData.addSensorData({.sensorID = sensorId,
//                               .sensorType = {"Temperature"},
//                               .status = (temps[i] == DEVICE_DISCONNECTED_C) ? 1 : 0,
//                               .unit = {"Celsius"},
//                               .timestamp = state.currentTime,
//                               .values = {temps[i]}});
//   }

//   // Optionally, handle error if any sensor is disconnected
//   if (!allValid)
//   {
//     logError("One or more DS18B20 sensors not connected or failed to read");
//     return false;
//   }
//   return true;
// }

bool readTDSData()
{
  state.tdsValue = tdsSensor.read(state.NFT_Res_Temp);                     // Use NFT_Res_Temp for temperature compensation
  int status = (state.tdsValue < 0 || state.tdsValue > 10000) ? 500 : 200; // Example status check

  sensorData.addSensorData({.sensorID = "TDS-1",
                            .sensorType = {"TDS"},
                            .status = status,
                            .unit = {"PPM"},
                            .timestamp = state.currentTime,
                            .values = {state.tdsValue}});

  // Update Latest Readings for Web Display
  latestReadings.tds = state.tdsValue;
  latestReadings.tdsTimestamp = state.currentTime;
  latestReadings.tdsStatus = status;
  latestReadings.hasValidData = (status == 200); // Set valid data flag based on status

  // Check if the TDS value is within a reasonable range
  if (state.tdsValue < 0 || state.tdsValue > 10000) // Example range check for TDS values
  {
    logError("Sensor Error from readTDSData (): Invalid TDS reading");
    state.sensorError = true;
    state.lastErrorTime = millis();
    status = status;                   // Update status to indicate error
    latestReadings.tdsStatus = status; // Update latest readings status

    return false;
  }
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(5000); // Delay for the serial monitor to start

  // Welcome Message
  Serial.println("\n\n[SYSTEM] Garden Guardian Firmware v1.0");

  // Record start time for stabilization tracking
  state.deviceStartTime = millis();
  Serial.print("[SYSTEM] Device start time: t=");
  Serial.println(state.deviceStartTime);

  // Configure watchdog
  esp_task_wdt_init(60, true); // 60 second timeout, panic on timeout
  esp_task_wdt_add(NULL);      // Add current task to WDT watch

  // Initialize Temperature Sensors
  // tempSensors.initialize();

  // Initialize Relays
  relay1.initialize();
  relay2.initialize();
  relay3.initialize();
  relay4.initialize();

  TDSController.initialize(); // Initialize TDS Controller Relay

  // Load and apply device settings BEFORE network initialization
  loadDeviceSettings();

  // Initialize Network
  Serial.println("[SYSTEM] Initializing network connections...");
  setupNetwork();
}

void loop()
{
  esp_task_wdt_reset();
  unsigned long currentMillis = millis();

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

      // Perform a initial Sensor Read
      Serial.println("[SYSTEM] Initial sensor read...");
      readSensorData();

      // Start web server when connected to WiFi
      network.startWebServer();
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
    if (currentMillis - state.lastReadingTime >= state.sensorRead_interval)
    {
      Serial.print("[SYSTEM] Time to take a sensor reading at t=");
      Serial.println(currentMillis);
      state.lastReadingTime = currentMillis;

      readSensorData();

      if (DEBUG_MODE)
      {
        // print all sensor data
        sensorData.printAllSensorData();

        // Clear the sensor data list after reading
        // sensorData.resetSensorData();
      }

      state.lastSensorRead = state.currentTime;
    }

    // Check if it's time to publish data via HTTP (non-blocking timer)
    if (state.httpPublishEnabled && network.isConnected() &&
        (currentMillis - state.lastHTTPPublishTime >= state.httpPublishInterval))
    {
      Serial.print("[HTTP] Time to publish sensor data at t=");
      Serial.println(currentMillis);
      state.lastHTTPPublishTime = currentMillis; // Only publish if we have data and device is stabilized

      if (sensorData.getSensorDataCount() > 0)
      {
        Serial.print("[HTTP] Publishing ");
        Serial.print(sensorData.getSensorDataCount());
        Serial.println(" sensor data items...");

        // Combine device_id with idCode for the full device identifier
        String fullDeviceID = state.deviceID + state.idCode;

        // Publish sensor data via HTTP
        bool publishSuccess = network.publishSensorData(sensorData, fullDeviceID);

        if (publishSuccess)
        {
          Serial.println("[HTTP] Data published successfully - clearing sensor data buffer");
          sensorData.resetSensorData(); // Clear data after successful publication
        }
        else
        {
          Serial.println("[HTTP] Failed to publish data - keeping data for next attempt");

          // FOR TESTING: Always Reset sensor data after attempting to publish
          sensorData.resetSensorData(); // Clear data after successful publication
        }
      }
      else
      {
        Serial.println("[HTTP] No data to publish or device not stabilized yet");
      }
    }

    // Check if it's time to check the TDS Controller
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

    // Handle web server requests when connected to WiFi
    if (network.isConnected())
    {
      network.handleClientRequestsWithSensorData(latestReadings);
    }

    break;

    // System Mode: Error
    // This mode is entered when a sensor error is detected
  case SystemMode::ERROR:
    break;

    // System Mode: Configuration
    // This mode is entered when the system is in AP mode or Web Server mode
  case SystemMode::CONFIG_MODE:

    // Check if it's time to read Data from connected Sensors
    if (state.lastSensorRead == 0 || state.currentTime - state.lastSensorRead >= state.sensorRead_interval)
    {
      Serial.print("[SYSTEM] Time to take a sensor reading at t=");
      Serial.println(currentMillis);
      state.lastReadingTime = currentMillis;

      readSensorData();

      state.lastSensorRead = state.currentTime;
    }

    // network.processDNSRequests(); // Process DNS first
    network.handleClientRequestsWithSensorData(latestReadings);
    break;
  }

  // Add a debug statement every 30 seconds to show the system is still running
  static unsigned long lastHeartbeat = 0;
  if (currentMillis - lastHeartbeat > 30000 && DEBUG_MODE)
  {
    Serial.println();
    Serial.print("[SYSTEM] Heartbeat at t=");
    Serial.print(currentMillis);
    Serial.print(", stabilized=");
    Serial.println(state.deviceStabilized ? "true" : "false");
    lastHeartbeat = currentMillis;

    // Print Current time from RTC
    Serial.print("[SYSTEM] Current time from RTC: ");
    Serial.print("Unix Epoch: ");
    Serial.println(state.currentTime);
    Serial.print("Formatted Time: ");
    struct tm *timeinfo = localtime((time_t *)&state.currentTime);
    Serial.print(asctime(timeinfo));

    // Network Status
    Serial.print("[NETWORK] Connected: ");
    Serial.println(network.isConnected() ? "Yes" : "No");
    Serial.print("[NETWORK] AP Mode: ");
    Serial.println(network.isAPMode() ? "Yes" : "No");

    Serial.println();
  }

  yield();
  delay(1000);
}
