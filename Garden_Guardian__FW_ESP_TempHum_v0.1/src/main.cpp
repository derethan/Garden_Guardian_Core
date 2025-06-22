// System Libraries
#include <Arduino.h>
#include <time.h>
#include "esp_sleep.h"

// Device Libraries
#include "dhtSensor.h"

// Config
#include "config.h"

DHTSensor dhtSensor(DHTPIN, DHTTYPE);

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
  delay(5000); // Allow time for serial to initialize
  Serial.println("\n\n[SYSTEM] Garden Guardian starting up...");

  // Record start time for stabilization tracking
  deviceStartTime = millis();
  Serial.print("[SYSTEM] Device start time: t=");
  Serial.println(deviceStartTime);

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

  Serial.print("[SYSTEM] Setup complete. Waiting for sensor stabilization (");
  Serial.print(SENSOR_STABILIZATION_TIME / 1000);
  Serial.println(" seconds)...");
}

void loop()
{
  unsigned long currentMillis = millis();

  // Check device stabilization status
  if (!deviceStabilized && currentMillis - deviceStartTime >= SENSOR_STABILIZATION_TIME)
  {
    deviceStabilized = true;
    Serial.print("[SYSTEM] Device stabilized at t=");
    Serial.print(currentMillis);
    Serial.println(", starting normal operation");

    // Take first official reading immediately after stabilization
    if (readSensorData(false))
    {
  
      Serial.println(currentMillis);
      lastReadingTime = currentMillis; // Update the reading time
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

  
  }

  // Add a debug statement every 30 seconds to show the system is still running
  static unsigned long lastHeartbeat = 0;
  if (currentMillis - lastHeartbeat > 30000)
  {
    Serial.print("[SYSTEM] Heartbeat at t=");
    Serial.print(currentMillis);
    Serial.print(", stabilized=");
    Serial.print(deviceStabilized ? "true" : "false");
    lastHeartbeat = currentMillis;
  }

  // Brief delay for serial to complete and to prevent busy-waiting
  delay(100);
}