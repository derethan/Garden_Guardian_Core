#include "wifiControl.h"
#include "getTime.h"
#include "controller.h" // Include the new controller header

WifiControl wifiCon;   // Create an instance of the WifiControl class
Controller controller; // Create an instance of the Controller class

const char *ssid = "BATECH_Camera"; // Replace with your SSID
const char *pass = "Norton66";      // Replace with your password
int wifiChannel = 0;

int32_t getWiFiChannel(const char *ssid)
{
  if (int32_t n = WiFi.scanNetworks())
  {
    for (uint8_t i = 0; i < n; i++)
    {
      if (!strcmp(ssid, WiFi.SSID(i).c_str()))
      {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

// Serial monitor command listener
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
    controller.processCommand(incomingMessage, wifiCon); // Use the new controller
  }
}

void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);
  delay(5000); // Delay to give time to start serial monitor

  // Welcome Message to Serial Monitor
  Serial.println("Welcome to the Garden Guardian Controller");

  // Set device as a Wi-Fi Station
  wifiCon.setMode(WIFI_AP_STA);

  // New Flow: Use funtion to get wifi channel for SSId, send the channel to the ESP-NOW Peer using default channel first
  // To have it update its channel, then connect to wifi


  // Connect to Wi-Fi
  wifiCon.connect(ssid, pass); // Pass ssid and pass to connect method
  wifiChannel = getWiFiChannel(ssid);

  // WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  // WiFi.printDiag(Serial); // Uncomment to verify channel change after

  delay(1000);

  // Init ESP-NOW
  wifiCon.initESPNow();

  // Register peer
  wifiCon.addPeer(broadcastAddress, wifiChannel);

  // Command instructions
  Serial.println("Commands: (X is the relay number)");
  Serial.println("relayX on - Turn Relay X ON");
  Serial.println("relayX off - Turn Relay X OFF");
  Serial.println("relayX auto - Enable Auto Mode for Relay X");
}

void loop()
{
  listenForCommands();
}

