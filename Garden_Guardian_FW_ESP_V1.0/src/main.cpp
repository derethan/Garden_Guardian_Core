/*****************************************
 *  Imported Libraries and files
 *****************************************/
// Arduino Libraries
#include <Arduino.h>
#include "definitions.h"

// wifi for esp32
#include <WiFi.h>

// Sensitive Data
#include "secrets.h"

// import Directory Files
#include "relayControl.h"
#include "wifiControl.h"
#include "getTime.h"

/*****************************************\
 *  Initialize Modules and Variables
\*****************************************/
RelayControl relay1(RELAY_PIN_LIGHTS);
RelayControl relay2(RELAY_PIN_HEATER_WATER_1, 5.0);
RelayControl relay3(RELAY_PIN_HEATER_ROOM, 5.0);
RelayControl relay4(RELAY_PIN_PUMP_WATER_1);
RelayControl relays[] = {relay1, relay2, relay3, relay4};

WifiControl wifiControl(SECRET_SSID, SECRET_PASS);
TimeRetriever timeRetriever;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 15000; // 1 minute

// Setup function
void setup()
{
  Serial.begin(115200);

    // Delay 5 sec for startup messages
  delay(5000);

  // Initialize The Relays
  for (RelayControl &relay : relays)
  {
    relay.initialize();
  }

  // Connect to Wi-Fi
  wifiControl.connect();

  // Initialize Time
  timeRetriever.initialize();

}

/*****************************************\
 *   MAIN PROGRAM LOOP
\*****************************************/
void loop()
{
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Light Relay
    relay1.setRelayForSchedule(timeRetriever);

    // Water Heater Relays
    relay2.setRelayforTemp(20.0, 22.0); // Current/Target

    // Room Heater Relays
    relay3.setRelayforTemp(20.0, 25.0); // Current/Target

    // NFT Pump Relay
    relay4.setRelayForTimedIntervals();
  }

  // Add a delay to avoid rapid toggling
  delay(1000);
}