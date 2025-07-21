// System Libraries
#include <Arduino.h>
#include <time.h>
#include "esp_sleep.h"

// Include System Libraries
#include "networkConnections.h"

// Device Libraries
#include "dhtSensor.h"

// Config
#include "config.h"
#include "secrets.h"

// Base configuration system
#include "base/deviceConfig.h"
#include "tempHumDeviceConfig.h"

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
  SystemMode currentMode = SystemMode::INITIALIZING;

  // Sleep settings - now configurable
  uint64_t SLEEP_DURATION = 15ULL * 1000000ULL; // 15 seconds sleep (configurable)
  const unsigned long MIN_WAKE_DURATION = 5000; // 5 seconds minimum wake time
  unsigned long wakeStartTime = 0;              // Track wake period start

  // Add timing control for sensor readings - now configurable
  unsigned long deviceStartTime = 0;
  unsigned long lastReadingTime = 0;
  unsigned long sensorRead_interval = 30000; // 30 seconds between readings (configurable)

  unsigned long SENSOR_STABILIZATION_TIME = 15000; // 15 seconds stabilization (configurable)
  bool deviceStabilized = false;
  unsigned long lastSensorRead = 0;
  unsigned long lastPublishTime = 0;
  unsigned long lastHTTPPublishTime = 0; // New timer for HTTP publishing
  uint32_t lastTimeSyncEpoch = 0;

  unsigned long currentTime = 0;
  unsigned long lastWiFiRetry = 0;

  bool wasConnected = false;
  String deviceID = DEVICE_ID; // Will be overridden by config (configurable)
  String idCode = IDCODE;      // Unique device identifier (configurable)

  // HTTP Publishing settings (configurable)
  bool httpPublishEnabled = true;
  unsigned long httpPublishInterval = 30000; // 30 seconds default

  bool sensorError = false;
  uint32_t lastErrorTime = 0;
  String lastErrorMessage;

  bool apAlwaysOn = false;

} state;

/*****************************************
 * Global Objects
 *****************************************/
SensorDataManager sensorData;
NetworkConnections network; // Network management object
DHTSensor dhtSensor(DHTPIN, DHTTYPE);
LatestReadings latestReadings; // Store latest readings for web display

/*****************************************
 * Forward Declarations
 *****************************************/
void loadDeviceSettings();
void setupNetwork();
bool readSensorData(bool discardReading = false);
bool readDHTData(bool discardReading = false);
void logError(const char *message);

/*****************************************
 * Configuration Functions
 *****************************************/
void loadDeviceSettings()
{
  // Create device-specific settings applier
  TempHumDeviceSettingsApplier applier(state);

  // Use the base configuration system to load and apply settings
  BaseConfig::loadAndApplyDeviceSettings(network, &applier);
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
 * System Functions
 *****************************************/

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

bool readSensorData(bool discardReading)
{
  if (!readDHTData(discardReading))
  {
    logError("Sensor Error: Failed to read DHT data");
    state.sensorError = true;
    return false;
  }
  else
  {
    state.sensorError = false;
    return true;
  }
}

bool readDHTData(bool discardReading)
{
  Serial.println();
  Serial.println("--------- Data Collection Starting ---------");
  Serial.print("[SENSOR] Reading sensor data at t=");
  Serial.println(millis());

  // Read temperature and humidity
  float temp = dhtSensor.readTemperature();
  float hum = dhtSensor.readHumidity();
  int status = 200; // Default status

  // Check if readings are valid
  if (isnan(temp) || isnan(hum))
  {
    Serial.println("[SENSOR] ERROR: Failed to read from DHT sensor!");
    state.sensorError = true;
    state.lastErrorTime = millis();
    status = 500; // Update status to indicate error

    // Update latest readings with error status
    latestReadings.temperatureStatus = 500;
    latestReadings.humidityStatus = 500;
  }
  else
  {
    // Always update latest readings for web display (even if discarding for publishing)
    latestReadings.temperature = temp;
    latestReadings.humidity = hum;
    latestReadings.temperatureTimestamp = state.currentTime;
    latestReadings.humidityTimestamp = state.currentTime;
    latestReadings.temperatureStatus = status;
    latestReadings.humidityStatus = status;
    latestReadings.hasValidData = true;

    Serial.print("[SENSOR] Latest readings updated - Temp: ");
    Serial.print(temp);
    Serial.print("°C, Humidity: ");
    Serial.print(hum);
    Serial.println("%");
  }

  // Only update the stored values for publishing if we're not discarding the reading
  if (!discardReading)
  {
    Serial.println("[SENSOR] Reading stored and ready for transmission");

    sensorData.addSensorData({.sensorID = "Temperature-" + state.idCode,
                              .sensorType = {"Temperature"},
                              .status = status,
                              .unit = {"°C"},
                              .timestamp = state.currentTime,
                              .values = {temp}});

    sensorData.addSensorData({.sensorID = "Humidity-" + state.idCode,
                              .sensorType = {"Humidity"},
                              .status = status,
                              .unit = {"%"},
                              .timestamp = state.currentTime,
                              .values = {hum}});
  }
  else
  {
    Serial.println("[SENSOR] Reading discarded for publishing (device in stabilization period)");
    Serial.println("[SENSOR] But latest readings updated for web display");
  }

  Serial.println("--------- Data Collection Complete ---------");
  Serial.println();

  return !isnan(temp) && !isnan(hum);
}

void setup()
{
  Serial.begin(115200);
  delay(5000); // Allow time for serial to initialize
  Serial.println("\n\n[SYSTEM] Garden Guardian - Temperature & Humidity Monitor");

  // Record start time for stabilization tracking
  state.deviceStartTime = millis();
  Serial.print("[SYSTEM] Device start time: t=");
  Serial.println(state.deviceStartTime);

  // Initialize DHT sensor
  Serial.println("[SENSOR] Initializing DHT sensor...");
  if (!dhtSensor.begin())
  {
    Serial.println("[SENSOR] ERROR: Failed to connect to DHT sensor!");
    state.sensorError = true;
    state.lastErrorTime = millis();
  }
  else
  {
    Serial.println("[SENSOR] DHT sensor initialized successfully");
  }
  delay(1000);

  // Load and apply device settings BEFORE network initialization
  loadDeviceSettings();

  // Initialize Network
  Serial.println("[SYSTEM] Initializing network connections...");
  setupNetwork();

  Serial.print("[SYSTEM] Setup complete. Waiting for sensor stabilization (");
  Serial.print(state.SENSOR_STABILIZATION_TIME / 1000);
  Serial.println(" seconds)...");
}

void loop()
{
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

    // Check device stabilization status
    if (!state.deviceStabilized && currentMillis - state.deviceStartTime >= state.SENSOR_STABILIZATION_TIME)
    {
      state.deviceStabilized = true;
      Serial.print("[SYSTEM] DHT Sensor stabilized at t=");
      Serial.print(currentMillis);
      Serial.println(", DHT Sensor readings will begin.");
    }

    // Check if it's time to read Data from connected Sensors
    if (currentMillis - state.lastReadingTime >= state.sensorRead_interval)
    {
      Serial.print("[SYSTEM] Time to take a sensor reading at t=");
      Serial.println(currentMillis);
      state.lastReadingTime = currentMillis;

      // Read sensor, but discard data if not yet stabilized
      bool discardReading = !state.deviceStabilized;
      if (discardReading)
      {
        Serial.println("[SYSTEM] Device not stabilized yet, reading will be discarded");
      }

      bool readSuccess = readSensorData(discardReading);

      if (DEBUG_MODE)
      {
        // print all sensor data
        sensorData.printAllSensorData();
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
      if (state.deviceStabilized && sensorData.getSensorDataCount() > 0)
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

    // Handle web server requests when connected to WiFi
    if (network.isConnected())
    {
      network.handleClientRequestsWithSensorData(latestReadings);
    }

    break;

    // System Mode: Error
    // This mode is entered when a sensor error is detected
  case SystemMode::ERROR:
    break; // System Mode: Configuration
    // This mode is entered when the system is in AP mode or Web Server mode
  case SystemMode::CONFIG_MODE:

    // Check device stabilization status
    if (!state.deviceStabilized && currentMillis - state.deviceStartTime >= state.SENSOR_STABILIZATION_TIME)
    {
      state.deviceStabilized = true;
      Serial.print("[SYSTEM] DHT Sensor stabilized at t=");
      Serial.print(currentMillis);
      Serial.println(", DHT Sensor readings will begin.");
    }

    // Check if it's time to read Data from connected Sensors (even in config mode)
    if (currentMillis - state.lastReadingTime >= state.sensorRead_interval)
    {
      Serial.print("[SYSTEM] Time to take a sensor reading at t=");
      Serial.println(currentMillis);
      state.lastReadingTime = currentMillis;

      // Read sensor, but discard data if not yet stabilized
      bool discardReading = !state.deviceStabilized;
      if (discardReading)
      {
        Serial.println("[SYSTEM] Device not stabilized yet, reading will be discarded");
      }

      bool readSuccess = readSensorData(discardReading);
      state.lastSensorRead = state.currentTime;
    }

    // Process DNS first
    // network.processDNSRequests();
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

  // Brief delay for serial to complete and to prevent busy-waiting
  delay(100);
}