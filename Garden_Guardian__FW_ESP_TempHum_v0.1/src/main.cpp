// System Libraries
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// Device Libraries
#include "dhtSensor.h"

// Config
#include "config.h"

DHTSensor dhtSensor(DHTPIN, DHTTYPE);

// Replace with the MAC address of the main ESP32 unit (receiver)
uint8_t mainESP32_MAC[] = {0xCC, 0xDB, 0xA7, 0x32, 0x24, 0xFC};

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
const unsigned long CONNECTION_RETRY_INTERVAL = 5000; // 5 seconds

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

  espNowConnected = true;
  Serial.println("Peer added");
}

void setup()
{
  Serial.begin(115200);

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
  ESPNow_addPeer(mainESP32_MAC);

  // Initialize DHT sensor
  if (!dhtSensor.begin())
  {
    Serial.println("Failed to connect to DHT sensor!");
    strcpy(sensorData.message, "DHT Sensor Error");

    // Todo: If connected, otherwise log locally
    esp_now_send(mainESP32_MAC, (uint8_t *)&sensorData, sizeof(sensorData)); // Send error message
  }
}

void loop()
{
  unsigned long currentMillis = millis();

  // Read temperature and humidity
  sensorData.temperature = dhtSensor.readTemperature();
  sensorData.humidity = dhtSensor.readHumidity();
  strcpy(sensorData.message, "OK"); // Normal status

  // Print data
  Serial.print("--------------------\n");
  Serial.print("Temperature: ");
  Serial.print(sensorData.temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(sensorData.humidity);
  Serial.println("%");
  Serial.print("--------------------\n");

  // ESP Now Not connected State
  if (!espNowConnected)
  {
    // Attempt reconnection at intervals
    if ((currentMillis - lastConnectionAttempt >= CONNECTION_RETRY_INTERVAL))
    {
      lastConnectionAttempt = currentMillis;

      // Attempt to reconnect
      if (esp_now_send(mainESP32_MAC, (uint8_t *)&sensorData, sizeof(sensorData)) == ESP_OK)
      {
        espNowConnected = true;
        return;
      }
    }

    // Store the data locally
    Serial.println("ESP-NOW not connected, storing data");
    // TODO: Add your local storage logic here
  }

  // ESP Now connected State
  if (espNowConnected)
  {
    esp_err_t result = esp_now_send(mainESP32_MAC, (uint8_t *)&sensorData, sizeof(sensorData));

    if (result != ESP_OK)
    {
      Serial.println("Error sending ESP-NOW data");
      espNowConnected = false;
    }
  }

  // Delay before next transmission
  delay(5000);
}

// void loop()
// {
//   // Read temperature and humidity
//   sensorData.temperature = dhtSensor.readTemperature();
//   sensorData.humidity = dhtSensor.readHumidity();
//   strcpy(sensorData.message, "OK"); // Normal status

//   // Send data via ESP-NOW
//   esp_err_t result = esp_now_send(mainESP32_MAC, (uint8_t *)&sensorData, sizeof(sensorData));

//   if (result == ESP_OK)
//   {
//     Serial.println("ESP-NOW Data Sent Successfully");
//   }
//   else
//   {
//     Serial.println("Error sending ESP-NOW data");
//   }

//   // Delay before next transmission
//   delay(5000);
// }
