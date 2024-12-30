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

bool sendData(struct_message *message, uint8_t *peerAddress)
{
  esp_err_t result = esp_now_send(peerAddress, (uint8_t *)message, sizeof(struct_message));
  return result == ESP_OK;
}

void listenForCommands()
{
  String incomingMessage = "";

  while (Serial.available() > 0)
  {
    char incomingChar = Serial.read();
    // Add character to message string if not newline/carriage return
    if (incomingChar != '\n' && incomingChar != '\r')
    {
      incomingMessage += incomingChar;
    }
    delay(2); // Small delay to allow buffer to fil
  }

  // Process command if message received
  if (incomingMessage.length() > 0)
  {
    Serial.print("Received command: ");
    Serial.println(incomingMessage);

    // Convert to lowercase for easier comparison
    incomingMessage.toLowerCase();

    // Command parsing
    if (incomingMessage == "relay1 on")
    {
      Serial.println("Turning Relay 1 ON");

      // Set values to send
      myData.type = "relay4";
      myData.manualOverride = true;
      myData.relayState = true;

      // Send data to the GG_Relay
      if (sendData(&myData, broadcastAddress))
      {
        Serial.println("Data sent");
      }
      else
      {
        Serial.println("Data send failed");
      }
    }
    else if (incomingMessage == "relay1 off")
    {
      Serial.println("Turning Relay 1 OFF");

      // Set values to send
      myData.type = "relay4";
      myData.manualOverride = true;
      myData.relayState = false;

      // Send data to the GG_Relay
      if (sendData(&myData, broadcastAddress))
      {
        Serial.println("Data sent");
      }
      else
      {
        Serial.println("Data send failed");
      }
    }
    else if (incomingMessage == "relay1 auto")
    {
      // Enabling auto mode
      Serial.println("Enabling Auto Mode");

      // Set values to send
      myData.type = "relay3";
      myData.manualOverride = false;

      // Send data to the GG_Relay
      if (sendData(&myData, broadcastAddress))
      {
        Serial.println("Data sent");
      }
      else
      {
        Serial.println("Data send failed");
      }
    }
    else
    {
      Serial.println("Unknown command");
    }
  }
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
  listenForCommands();
}

// Set values to send
// strcpy(myData.timestamp, "2021-09-01 12:00:00");
// myData.type = "RELAY:light";
// myData.onHour = 6;
// myData.offHour = 18;

// if (sendData(&myData))
// {
//   Serial.println("Data sent");
// }
// else
// {
//   Serial.println("Data send failed");
// }