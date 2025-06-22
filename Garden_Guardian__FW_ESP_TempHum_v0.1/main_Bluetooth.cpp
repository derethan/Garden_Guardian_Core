// System Libraries
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <time.h>
#include "esp_sleep.h"

// Device Libraries
#include "dhtSensor.h"

// Config
#include "config.h"

// BLE UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TEMPERATURE_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define HUMIDITY_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"

// BLE global variables
BLEServer *pServer = nullptr;
BLECharacteristic *pTemperatureCharacteristic = nullptr;
BLECharacteristic *pHumidityCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

DHTSensor dhtSensor(DHTPIN, DHTTYPE);

//
// Structure to hold sensor data
typedef struct
{
  float temperature;
  float humidity;
  char message[32]; // For errors or status
} SensorData;

SensorData sensorData;

// Sleep settings
const uint64_t SLEEP_DURATION = 15ULL * 1000000ULL; // 15 seconds sleep
const unsigned long MIN_WAKE_DURATION = 5000;       // 5 seconds minimum wake time
unsigned long wakeStartTime = 0;                    // Track wake period start

// Add timing control for sensor readings
unsigned long deviceStartTime = 0;
unsigned long lastReadingTime = 0;
const unsigned long READING_INTERVAL = 30000; // 5 minutes between readings

const unsigned long SENSOR_STABILIZATION_TIME = 60000; // 1 minute stabilization
bool deviceStabilized = false;

// BLE server callbacks
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.print("[BLE] Device connected at t=");
    Serial.println(millis());
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.print("[BLE] Device disconnected at t=");
    Serial.println(millis());
  }
};

bool readSensorData(bool discardReading = false)
{
  Serial.print("[SENSOR] Reading sensor data at t=");
  Serial.println(millis());

  // Read temperature and humidity
  float temp = dhtSensor.readTemperature();
  float hum = dhtSensor.readHumidity();

  // Check if readings are valid
  if (isnan(temp) || isnan(hum))
  {
    Serial.println("[SENSOR] ERROR: Failed to read from DHT sensor!");
    strcpy(sensorData.message, "Reading Error");
    return false;
  }

  // Print data regardless of whether we're discarding it
  Serial.println("--------- SENSOR DATA ---------");
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print("°C, Humidity: ");
  Serial.print(hum);
  Serial.println("%");
  Serial.println("-------------------------------");

  // Only update the stored values if we're not discarding the reading
  if (!discardReading)
  {
    sensorData.temperature = temp;
    sensorData.humidity = hum;
    strcpy(sensorData.message, "OK");
    Serial.println("[SENSOR] Reading stored and ready for transmission");
  }
  else
  {
    Serial.println("[SENSOR] Reading discarded (device in stabilization period)");
  }

  return true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n[SYSTEM] Garden Guardian starting up...");

  // Record start time for stabilization tracking
  deviceStartTime = millis();
  Serial.print("[SYSTEM] Device start time: t=");
  Serial.println(deviceStartTime);

  // Debug delay
  Serial.println("[SYSTEM] Initial 5-second delay...");
  delay(5000);

    // Initialize DHT sensor
    Serial.println("[SENSOR] Initializing DHT sensor...");
    if (!dhtSensor.begin())
    {
      Serial.println("[SENSOR] ERROR: Failed to connect to DHT sensor!");
      strcpy(sensorData.message, "DHT Sensor Error");
    }
    else
    {
      Serial.println("[SENSOR] DHT sensor initialized successfully");
    }

    delay(1000);

  // Initialize BLE
  Serial.println("[BLE] Initializing BLE...");
  BLEDevice::init("GG-ENV");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics
  Serial.println("[BLE] Creating BLE characteristics...");
  pTemperatureCharacteristic = pService->createCharacteristic(
      TEMPERATURE_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  pHumidityCharacteristic = pService->createCharacteristic(
      HUMIDITY_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);

  // Create BLE Descriptor
  pTemperatureCharacteristic->addDescriptor(new BLE2902());
  pHumidityCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  Serial.println("[BLE] Starting advertisement...");
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("[BLE] BLE device ready to pair");



  Serial.print("[SYSTEM] Setup complete. Waiting for sensor stabilization (");
  Serial.print(SENSOR_STABILIZATION_TIME / 1000);
  Serial.println(" seconds)...");
}

void loop()
{
  unsigned long currentMillis = millis();

  // If this is the start of a wake cycle, record the time
  // if (wakeStartTime == 0)
  // {
  //   Serial.print("[SYSTEM] Device waking up at t=");
  //   Serial.println(currentMillis);
  //   wakeStartTime = currentMillis;
  // }

  // Check device stabilization status
  if (!deviceStabilized && currentMillis - deviceStartTime >= SENSOR_STABILIZATION_TIME)
  {
    deviceStabilized = true;
    Serial.print("[SYSTEM] Device stabilized at t=");
    Serial.print(currentMillis);
    Serial.println(", starting normal operation");

    // Take first official reading immediately after stabilization
    if (readSensorData(false) && deviceConnected)
    {
      Serial.println("[BLE] Sending first post-stabilization data");

      // Convert float to string for BLE transmission
      char tempStr[8];
      char humStr[8];
      dtostrf(sensorData.temperature, 6, 2, tempStr);
      dtostrf(sensorData.humidity, 6, 2, humStr);

      pTemperatureCharacteristic->setValue(tempStr);
      pHumidityCharacteristic->setValue(humStr);
      pTemperatureCharacteristic->notify();
      pHumidityCharacteristic->notify();

      Serial.print("[BLE] First data sent via BLE at t=");
      Serial.println(currentMillis);
      lastReadingTime = currentMillis; // Update the reading time
    }
    else if (!deviceConnected)
    {
      Serial.println("[BLE] No device connected, data not sent");
    }
  }

  // Read sensors at regular intervals regardless of stabilization
  // But only store/send data if we've passed stabilization
  if (currentMillis - lastReadingTime >= READING_INTERVAL)
  {
    Serial.print("[SYSTEM] Time to take a sensor reading at t=");
    Serial.println(currentMillis);
    lastReadingTime = currentMillis;

    // Read sensor, but discard data if not yet stabilized
    bool discardReading = !deviceStabilized;
    if (discardReading)
    {
      Serial.println("[SYSTEM] Device not stabilized yet, reading will be discarded");
    }

    bool readSuccess = readSensorData(discardReading);

    // Only send data if stabilized, reading successful, and connected
    if (!discardReading && readSuccess && deviceConnected)
    {
      Serial.println("[BLE] Sending regular interval data");

      // Convert float to string for BLE transmission
      char tempStr[8];
      char humStr[8];
      dtostrf(sensorData.temperature, 6, 2, tempStr);
      dtostrf(sensorData.humidity, 6, 2, humStr);

      pTemperatureCharacteristic->setValue(tempStr);
      pHumidityCharacteristic->setValue(humStr);
      pTemperatureCharacteristic->notify();
      pHumidityCharacteristic->notify();

      Serial.print("[BLE] Data sent via BLE at t=");
      Serial.println(currentMillis);
    }
    else if (!deviceConnected && !discardReading && readSuccess)
    {
      Serial.println("[BLE] No device connected, data not sent");
    }
  }

  // Handle BLE connection status changes
  if (!deviceConnected && oldDeviceConnected)
  {
    Serial.print("[BLE] Connection lost, restarting advertising at t=");
    Serial.println(currentMillis);
    delay(500);                  // Give the bluetooth stack time to get ready
    pServer->startAdvertising(); // Restart advertising
    Serial.println("[BLE] Advertisement restarted");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected)
  {
    Serial.print("[BLE] New connection established at t=");
    Serial.println(currentMillis);
    oldDeviceConnected = deviceConnected;
  }

  // Only enter sleep mode if minimum wake time has passed
  // if (currentMillis - wakeStartTime >= MIN_WAKE_DURATION)
  // {
  //   Serial.print("[SYSTEM] Going to sleep for ");
  //   Serial.print(SLEEP_DURATION / 1000000ULL);
  //   Serial.print(" seconds at t=");
  //   Serial.println(currentMillis);
  //   delay(100); // Brief delay for serial to complete
  //
  //   esp_sleep_enable_timer_wakeup(SLEEP_DURATION);
  //   wakeStartTime = 0; // Reset wake start time
  //   esp_light_sleep_start();
  // }

  // Add a debug statement every 30 seconds to show the system is still running
  static unsigned long lastHeartbeat = 0;
  if (currentMillis - lastHeartbeat > 30000)
  {
    Serial.print("[SYSTEM] Heartbeat at t=");
    Serial.print(currentMillis);
    Serial.print(", stabilized=");
    Serial.print(deviceStabilized ? "true" : "false");
    Serial.print(", connected=");
    Serial.println(deviceConnected ? "true" : "false");
    lastHeartbeat = currentMillis;
  }

  // Brief delay for serial to complete and to prevent busy-waiting
  delay(100);
}