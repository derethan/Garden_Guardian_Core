#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// Structure to hold received data
typedef struct
{
  float temperature;
  float humidity;
  char message[32]; // For error message or status
} SensorData;

// Callback function to handle incoming data
void OnDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  SensorData *sensorData = (SensorData *)data;

  Serial.println("Received Data:");
  Serial.print("Temperature: ");
  Serial.println(sensorData->temperature);
  Serial.print("Humidity: ");
  Serial.println(sensorData->humidity);
  Serial.print("Message: ");
  Serial.println(sensorData->message);
}

void setup()
{
  Serial.begin(115200);

  // Initialize Wi-Fi as Station mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  delay(100);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW Initialization Failed!");
    return;
  }

  // Register the callback function to handle incoming data
  esp_now_register_recv_cb(OnDataReceived);

  Serial.println("Waiting for incoming data...");
}

void loop()
{
  // Nothing to do here, we're waiting for data to be received
}
