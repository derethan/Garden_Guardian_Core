// System Libraries
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <time.h>
#include "esp_sleep.h"

// Device Libraries
#include "dhtSensor.h"

// Config
#include "config.h"

DHTSensor dhtSensor(DHTPIN, DHTTYPE);

uint8_t gardenGuardian_MAC[] = {0xCC, 0xDB, 0xA7, 0x32, 0x24, 0xFC};

// Structure to hold sensor data
typedef struct
{
  float temperature;
  float humidity;
  char message[32]; // For errors or status
} SensorData;

SensorData sensorData;

// Add global connection state
bool espNowConnected = false;
unsigned long lastConnectionAttempt = 0;
const unsigned long CONNECTION_RETRY_INTERVAL = 300000; // 5 seconds

// Sleep settings
const uint64_t SLEEP_DURATION = 15ULL * 1000000ULL; // 15 seconds sleep
const unsigned long MIN_WAKE_DURATION = 5000;       // 5 seconds minimum wake time
unsigned long wakeStartTime = 0;                    // Track wake period start

// Add timing control for sensor readings
unsigned long deviceStartTime = 0;
unsigned long lastReadingTime = 0;
const unsigned long READING_INTERVAL = 300000; // 5 minutes between readings

const unsigned long SENSOR_STABILIZATION_TIME = 60000; // 1 minute stabilization
bool deviceStabilized = false;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("ESP-NOW Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");

  espNowConnected = (status == ESP_NOW_SEND_SUCCESS);
}

void ESPNow_addPeer(const uint8_t *macAddress)
{
  // Add peer (main ESP32 unit)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer!");
    espNowConnected = false;
    return;
  }

  Serial.println("Peer added to peers list - connection pending verification");
}

bool readSensorData(bool discardReading = false)
{
  // Read temperature and humidity
  float temp = dhtSensor.readTemperature();
  float hum = dhtSensor.readHumidity();

  // Check if readings are valid
  if (isnan(temp) || isnan(hum))
  {
    Serial.println("Failed to read from DHT sensor!");
    strcpy(sensorData.message, "Reading Error");
    return false;
  }

  // Print data regardless of whether we're discarding it
  Serial.println("--------------------");
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print("°C, Humidity: ");
  Serial.print(hum);
  Serial.println("%");

  // Only update the stored values if we're not discarding the reading
  if (!discardReading)
  {
    sensorData.temperature = temp;
    sensorData.humidity = hum;
    strcpy(sensorData.message, "OK");
    Serial.println("Reading stored and ready for transmission");
  }
  else
  {
    Serial.println("Reading discarded (device in stabilization period)");
  }

  return true;
}

void setup()
{
  Serial.begin(115200);

  // Record start time for stabilization tracking
  deviceStartTime = millis();

  // Debug delay
  delay(5000);

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Add peer (main ESP32 unit)
  ESPNow_addPeer(gardenGuardian_MAC);

  // Send an initial test message to verify connection
  strcpy(sensorData.message, "Init");
  sensorData.temperature = 0;
  sensorData.humidity = 0;
  esp_now_send(gardenGuardian_MAC, (uint8_t *)&sensorData, sizeof(sensorData));
  Serial.println("Sent initial connection test message");

  // Initialize DHT sensor
  if (!dhtSensor.begin())
  {
    Serial.println("Failed to connect to DHT sensor!");
    strcpy(sensorData.message, "DHT Sensor Error");

    if (espNowConnected)
    {
      esp_now_send(gardenGuardian_MAC, (uint8_t *)&sensorData, sizeof(sensorData));
    }
  }

  Serial.println("Setup complete. Waiting for sensor stabilization...");
}

void loop()
{
  unsigned long currentMillis = millis();

  // If this is the start of a wake cycle, record the time
  if (wakeStartTime == 0)
  {
    Serial.println("Device waking up...");
    wakeStartTime = currentMillis;
  }

  // Check device stabilization status
  if (!deviceStabilized && currentMillis - deviceStartTime >= SENSOR_STABILIZATION_TIME)
  {
    deviceStabilized = true;
    Serial.println("Device stabilized, starting normal operation");
    // Take first official reading immediately after stabilization
    if (readSensorData(false) && espNowConnected)
    {
      esp_now_send(gardenGuardian_MAC, (uint8_t *)&sensorData, sizeof(sensorData));
      Serial.println("First post-stabilization data sent to Garden Guardian");
      lastReadingTime = currentMillis; // Update the reading time
    }
  }


  // Handle ESP-NOW connection status
  if (!espNowConnected)
  {
    // Attempt reconnection at intervals
    if (currentMillis - lastConnectionAttempt >= CONNECTION_RETRY_INTERVAL)
    {
      Serial.println("Attempting to reconnect...");
      Serial.print("--------------------\n");

      lastConnectionAttempt = currentMillis;

      // Send current data or connection message
      strcpy(sensorData.message, "Reconnecting");
      esp_now_send(gardenGuardian_MAC, (uint8_t *)&sensorData, sizeof(sensorData));
    }
  }

  // Read sensors at regular intervals regardless of stabilization
  // But only store/send data if we've passed stabilization
  if (currentMillis - lastReadingTime >= READING_INTERVAL)
  {
    Serial.println("Time to take a sensor reading");
    lastReadingTime = currentMillis;

    // Read sensor, but discard data if not yet stabilized
    bool discardReading = !deviceStabilized;
    bool readSuccess = readSensorData(discardReading);

    // Only send data if stabilized, reading successful, and connected
    if (!discardReading && readSuccess && espNowConnected)
    {
      esp_now_send(gardenGuardian_MAC, (uint8_t *)&sensorData, sizeof(sensorData));
      Serial.println("Data sent to Garden Guardian");
    }
  }


    // Only enter sleep mode if minimum wake time has passed
    if (currentMillis - wakeStartTime >= MIN_WAKE_DURATION)
    {
      Serial.println("Going to sleep for 15 seconds...");
      delay(100); // Brief delay for serial to complete
      
      esp_sleep_enable_timer_wakeup(SLEEP_DURATION);
      wakeStartTime = 0; // Reset wake start time
      esp_light_sleep_start();
    }

  // Delay for a moment to allow serial to complete
  delay(100);
}