#include "controller.h"

// Create a struct_message called myData
extern struct_message myData;
extern uint8_t broadcastAddress[6];

void Controller::processCommand(String incomingMessage, WifiControl &wifiCon)
{
    Serial.print("Received command: ");
    Serial.println(incomingMessage);

    // Convert to lowercase for easier comparison
    incomingMessage.toLowerCase();

    // Command parsing
    if (incomingMessage.startsWith("relay"))
    {
        int relayNumber = incomingMessage.charAt(5) - '0';
        String command = incomingMessage.substring(7);

        if (relayNumber >= 1 && relayNumber <= 4)
        {
            Serial.print("Relay ");
            Serial.print(relayNumber);
            Serial.print(" command: ");
            Serial.println(command);

            // Set values to send
            myData.type = "relay" + String(relayNumber);
            myData.manualOverride = true;

            if (command == "on")
            {
                Serial.print("Turning Relay ");
                Serial.print(relayNumber);
                Serial.println(" ON");
                myData.relayState = true;
            }
            else if (command == "off")
            {
                Serial.print("Turning Relay ");
                Serial.print(relayNumber);
                Serial.println(" OFF");
                myData.relayState = false;
            }
            else if (command == "auto")
            {
                Serial.print("Enabling Auto Mode for Relay ");
                Serial.println(relayNumber);
                myData.manualOverride = false;

                if (relayNumber == 3)
                {
                    Serial.println("Setting Relay 3 to Interval Timer Mode");
                    // Interval Timer
                    myData.onInterval = 5;
                    myData.offInterval = 15;

                    // Print the Settings
                    Serial.print("On Interval: ");
                    Serial.println(myData.onInterval);
                    Serial.print("Off Interval: ");
                    Serial.println(myData.offInterval);
                }
                else if (relayNumber == 4)
                {
                    // Time-based Timer
                    Serial.println("Setting Relay 4 to Time-based Timer Mode");

                    // Get current time in unix timestamp
                    String currentTime = wifiCon.getFormattedTime();

                    myData.onHour = 6;
                    myData.offHour = 18;
                    myData.timestamp = currentTime;

                }
            }
            else
            {
                Serial.println("Unknown command");
                return;
            }

            // Send data to the GG_Relay
            if (wifiCon.sendData(&myData, broadcastAddress))
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
            Serial.println("Invalid relay number");
        }
    }
    else
    {
        Serial.println("Unknown command");
    }
}
