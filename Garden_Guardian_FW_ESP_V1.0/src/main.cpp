/*****************************************
 *  Imported Libraries and files
 *****************************************/
// Arduino Libraries
#include <Arduino.h>
#include "definitions.h"

// Sensitive Data
#include "secrets.h"

// import Directory Files
#include "relayControl.h"

/*****************************************
 *  Initialize Modules and Variables
 *****************************************/
RelayControl relay1(RELAY_PIN_1, 5.0);
RelayControl relay2(RELAY_PIN_2, 5.0);
RelayControl relay3(RELAY_PIN_3, 5.0);
RelayControl relay4(RELAY_PIN_4, 5.0);
RelayControl relays[] = {relay1, relay2, relay3, relay4};

// Setup function
void setup()
{
  Serial.begin(115200);

  // Initialize The Relays
  for (RelayControl &relay : relays)
  {
    relay.initialize();
  }

  // Delay 5 sec for startup messages
  delay(5000);

  // Print relay messages
  Serial.println("Enter On to turn on the relay, or Off to turn off the relay");
  
}

void loop()
{
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    command.toLowerCase();  // Convert to lowercase for easier comparison

    // Split command into relay number and action
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex == -1) {
      Serial.println("Invalid command format. Use: relay[1-4] [on/off] or all [on/off]");
      return;
    }

    String relay = command.substring(0, spaceIndex);
    String action = command.substring(spaceIndex + 1);

    if (relay == "all") {
      for (RelayControl &relay : relays) {
        if (action == "on") {
          relay.turnOn();
        } else if (action == "off") {
          relay.turnOff();
        }
      }
      Serial.println("All relays turned " + action);
      return;
    }

    int relayNum = -1;
    if (relay == "relay1") relayNum = 0;
    else if (relay == "relay2") relayNum = 1;
    else if (relay == "relay3") relayNum = 2;
    else if (relay == "relay4") relayNum = 3;

    if (relayNum >= 0 && relayNum < 4) {
      if (action == "on") {
        relays[relayNum].turnOn();
        Serial.println("Relay " + String(relayNum + 1) + " turned on");
      } else if (action == "off") {
        relays[relayNum].turnOff();
        Serial.println("Relay " + String(relayNum + 1) + " turned off");
      } else {
        Serial.println("Invalid action. Use 'on' or 'off'");
      }
    } else {
      Serial.println("Invalid relay number. Use relay1, relay2, relay3, relay4, or all");
    }
  }
}