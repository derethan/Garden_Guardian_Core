// CC:DB:A7:32:7:BC

/*****************************************
 *  Imported Libraries and files
 *****************************************/
// Arduino Libraries
#include <Arduino.h>
#include "definitions.h"

// wifi for esp32
#include <esp_now.h>
#include <WiFi.h>

// Sensitive Data
#include "secrets.h"

// import Directory Files
#include "relayControl.h"

/*****************************************\
 *  Initialize Modules and Variables
\*****************************************/
RelayControl relay1(RELAY_PIN_LIGHTS);
RelayControl relay2(RELAY_PIN_HEATER_WATER_1, 5.0);
RelayControl relay3(RELAY_PIN_HEATER_ROOM, 5.0);
RelayControl relay4(RELAY_PIN_PUMP_WATER_1);
RelayControl relays[] = {relay1, relay2, relay3, relay4};

// Time Variables For Light Relay
int onHour = 6;   // 6 AM
int offHour = 18; // 6 PM
String currentTime;

// Temperature Variables for heater Relays
float currentWaterTemp = 0.0;
float targetWaterTemp = 20.0;

float currentHeaterTemp = 0.0;
float targetHeaterTemp = 24.0;

// Pump Variables (Minutes)
int onInterval = 15;
int offInterval = 30;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 15000; // 1 minute

// Must match the sender structure
typedef struct epsNowMessage
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

} epsNowMessage;

// Create a struct_message called myData
epsNowMessage recievedMessage;

void parseData(epsNowMessage message)
{
  // Check the type of message
  if (message.type == "RELAY:light")
  {

    // Check if the manual override is set
    if (message.manualOverride)
    {
      relay1.setManualOverride(true);
      if (message.relayState)
      {
        relay1.turnOn();
      }
      else
      {
        relay1.turnOff();
      }
    }
    else
    {
      relay1.setManualOverride(false);
    }

    // Print Message
    Serial.println("Relay Light Command received");
    currentTime = message.timestamp;
    onHour = message.onHour;
    offHour = message.offHour;
  }
  else if (message.type = "RELAY:waterHeater")
  {
    // Check if the manual override is set
    if (message.manualOverride)
    {
      relay2.setManualOverride(true);
      if (message.relayState)
      {
        relay2.turnOn();
      }
      else
      {
        relay2.turnOff();
      }
    }
    else
    {
      relay2.setManualOverride(false);
    }

    // Print Message
    Serial.println("Water Heater Data Received");
    currentWaterTemp = message.currentTemp;
    targetWaterTemp = message.targetTemp;
  }
  else if (message.type = "RELAY:roomHeater")
  {
    // Check if the manual override is set
    if (message.manualOverride)
    {
      relay3.setManualOverride(true);
      if (message.relayState)
      {
        relay3.turnOn();
      }
      else
      {
        relay3.turnOff();
      }
    }
    else
    {
      relay3.setManualOverride(false);
    }

    // Print Message
    Serial.println("Room Heater Data Received");
    currentHeaterTemp = message.currentTemp;
    targetHeaterTemp = message.targetTemp;
  }
  else if (message.type = "RELAY:pump")
  {
    // Check if the manual override is set
    if (message.manualOverride)
    {
      relay4.setManualOverride(true);
      if (message.relayState)
      {
        relay4.turnOn();
      }
      else
      {
        relay4.turnOff();
      }
    }
    else
    {
      relay4.setManualOverride(false);
    }

    // Print Message
    Serial.println("Pump Data Received");
    onInterval = message.onInterval;
    offInterval = message.offInterval;
  }
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&recievedMessage, incomingData, sizeof(recievedMessage));
  parseData(recievedMessage);
}

// Setup function
void setup()
{
  Serial.begin(115200);

  // Delay 5 sec for startup messages
  delay(5000);

  // Startup message
  Serial.println("ESP32 Board Initialized");

  // Initialize The Relays
  for (RelayControl &relay : relays)
  {
    relay.initialize();
  }

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

/*****************************************\
 *   MAIN PROGRAM LOOP
\*****************************************/
void loop()
{

  // Relay Check
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    // Message
    Serial.println("Checking Relays");

    // Light Relay
    relay1.setRelayForSchedule(onHour, offHour, currentTime);

    // Water Heater Relays
    relay2.setRelayforTemp(currentWaterTemp, targetWaterTemp); // Current/Target

    // Room Heater Relays
    relay3.setRelayforTemp(currentHeaterTemp, targetHeaterTemp); // Current/Target

    // NFT Pump Relay
    relay4.setRelayForTimedIntervals(onInterval, offInterval);

    previousMillis = currentMillis;
  }

  // Add a delay to avoid rapid toggling
  delay(1000);
}