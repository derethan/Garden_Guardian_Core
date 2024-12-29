// /*****************************************
//  *  Imported Libraries and files
//  *****************************************/
// // Arduino Libraries
// #include <Arduino.h>
// #include "definitions.h"

// // wifi for esp32
// #include <WiFi.h>

// // Sensitive Data
// #include "secrets.h"

// // import Directory Files
// #include "wifiControl.h"
// #include "getTime.h"

// /*****************************************\
//  *  Initialize Modules and Variables
// \*****************************************/

// WifiControl wifiControl(SECRET_SSID, SECRET_PASS);
// TimeRetriever timeRetriever;

// // Timer variables
// unsigned long previousMillis = 0;
// const long interval = 15000; // 1 minute

// // Setup function
// void setup()
// {
//   Serial.begin(115200);

//   // Delay 5 sec for startup messages
//   delay(5000);

//   // Connect to Wi-Fi
//   // wifiControl.connect();

//   // Initialize Time
//   timeRetriever.initialize();

//   // ESP Now
//   wifiControl.espConnect();
// }

// /*****************************************\
//  *   MAIN PROGRAM LOOP
// \*****************************************/
// void loop()
// {
//   // Get the Mac Address of the ESP32
//   // String macAddress = wifiControl.getMacAddress();
//   // Serial.println(macAddress);

//   // Relay Check
//   unsigned long currentMillis = millis();
//   if (currentMillis - previousMillis >= interval)
//   {
//     previousMillis = currentMillis;
//   }

//   // Add a delay to avoid rapid toggling
//   delay(1000);
// }

#include <esp_now.h>
#include <WiFi.h>

// uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[] = {0xCC, 0xDB, 0xA7, 0x32, 0x07, 0xBC};

// Must match the receiver structure
typedef struct struct_message
{
  char timestamp[32];
  String type;
  int onHour;
  int offHour;
  float currentTemp;
  float targetTemp;
  int onInterval;
  int offInterval;
  bool manualOverride;
  bool relayState;

} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);
  delay(5000); // Delay to give time to start serial monitor
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop()
{

  // Set values to send
  strcpy(myData.timestamp, "2021-09-01 12:00:00");
  myData.type = "RELAY:light";
  myData.onHour = 6;
  myData.offHour = 18;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
  delay(15000);
}
