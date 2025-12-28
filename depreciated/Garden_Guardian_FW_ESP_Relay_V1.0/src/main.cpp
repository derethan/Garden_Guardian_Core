// CC:DB:A7:32:7:BC

/*****************************************
 *  Imported Libraries and files
 *****************************************/
// Arduino Libraries
#include <Arduino.h>
#include "definitions.h"

// wifi for esp32
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// Sensitive Data
#include "secrets.h"

// import Directory Files
#include "relayControl.h"

// Update these constants near the top of the file
#define TIME_ZONE_OFFSET -3.5  // NST (UTC-3:30)
bool isDST = false;  // This will be managed dynamically

/*****************************************\
 *  Initialize Modules and Variables
\*****************************************/
RelayControl relay1(RELAY_PIN_HEATER_WATER_1, 5.0);  // Water Temperature
RelayControl relay2(RELAY_PIN_HEATER_ROOM, 5.0);     // Room Heater
RelayControl relay3(RELAY_PIN_PUMP_WATER_1);         // Water Pump
RelayControl relay4(RELAY_PIN_LIGHTS);               // Lights
RelayControl relays[] = {relay1, relay2, relay3, relay4};

// Time Variables For Light Relay
int onHour = 18;   // 4 AM
int offHour = 12; // 10 PM
String currentTime;

// Temperature Variables for heater Relays
float currentWaterTemp = 0.0;
float targetWaterTemp = 20.0;

float currentHeaterTemp = 0.0;
float targetHeaterTemp = 24.0;

// Pump Variables (Minutes)
int onInterval = 10;
int offInterval = 15;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 15000; // 1 minute

// Must match the sender structure
typedef struct epsNowMessage
{
  String timestamp;
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
void setDeviceTime(uint32_t unixTime)
{
  struct timeval tv;
  tv.tv_sec = unixTime; // Unix timestamp (seconds since Jan 1, 1970)
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
}
void parseData(epsNowMessage message)
{
  // Debugging: Print the received message details
  Serial.println("Received Message:");
  Serial.print("Timestamp: ");
  Serial.println(message.timestamp);
  Serial.print("Type: ");
  Serial.println(message.type);
  Serial.print("On Hour: ");
  Serial.println(message.onHour);
  Serial.print("Off Hour: ");
  Serial.println(message.offHour);
  Serial.print("Current Temp: ");
  Serial.println(message.currentTemp);
  Serial.print("Target Temp: ");
  Serial.println(message.targetTemp);
  Serial.print("On Interval: ");
  Serial.println(message.onInterval);
  Serial.print("Off Interval: ");
  Serial.println(message.offInterval);
  Serial.print("Manual Override: ");
  Serial.println(message.manualOverride);
  Serial.print("Relay State: ");
  Serial.println(message.relayState);

  // Check the type of message
  if (message.type == "relay1")
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
    Serial.println("Relay 1 Data Processed");
    currentHeaterTemp = message.currentTemp;
    targetHeaterTemp = message.targetTemp;
  }
  else if (message.type == "relay2")
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
    Serial.println("Relay 2 Data Processed");
    currentWaterTemp = message.currentTemp;
    targetWaterTemp = message.targetTemp;
  }
  else if (message.type == "relay3")
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
    Serial.println("Relay 3 Data Processed");
    onInterval = message.onInterval;
    offInterval = message.offInterval;
  }
  else if (message.type == "relay4")
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
    Serial.println("Relay 4 Data Processed");
    // currentTime = message.timestamp;
    onHour = message.onHour;
    offHour = message.offHour;

    // Get the Unix TimeStamp
    currentTime = message.timestamp;

    // Set the time variables
  }
  else
  {
    Serial.println("Invalid Message Type");
  }
}

// Insert your SSID
constexpr char WIFI_SSID[] = "BATECH_Camera";

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

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&recievedMessage, incomingData, sizeof(recievedMessage));
  parseData(recievedMessage);
}

// Add these functions after the includes and before the setup():
bool isNewfoundlandDST() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }

    // DST starts second Sunday in March at 2:00 AM
    // DST ends first Sunday in November at 2:00 AM

    int month = timeinfo.tm_mon + 1; // tm_mon is 0-based
    int day = timeinfo.tm_mday;
    int wday = timeinfo.tm_wday; // 0 = Sunday
    int hour = timeinfo.tm_hour;

    // January, February, December are never DST
    if (month < 3 || month > 11) {
        return false;
    }
    
    // April to October are always DST
    if (month > 3 && month < 11) {
        return true;
    }

    int weekOfMonth = (day - 1) / 7 + 1;

    // March - DST starts on second Sunday at 2:00 AM
    if (month == 3) {
        // Before second Sunday
        if (weekOfMonth < 2) return false;
        // After second Sunday
        if (weekOfMonth > 2) return true;
        // On second Sunday
        return (wday == 0 && hour >= 2) || wday > 0;
    }
    
    // November - DST ends on first Sunday at 2:00 AM
    if (month == 11) {
        // After first Sunday
        if (weekOfMonth > 1) return false;
        // Before first Sunday
        if (wday < 0) return true;
        // On first Sunday
        return hour < 2;
    }

    return false;
}

void updateTimeConfig() {
    bool newDST = isNewfoundlandDST();
    if (newDST != isDST) {
        isDST = newDST;
        configTime((TIME_ZONE_OFFSET * 3600), (isDST ? 3600 : 0), "pool.ntp.org");
        Serial.print("DST status changed to: ");
        Serial.println(isDST ? "ON" : "OFF");
    }
}

// Replace the connectToWiFi() function with this updated version
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(SECRET_SSID, SECRET_PASS);  // WIFI_PASSWORD should be defined in secrets.h
  delay(1000);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Initial time configuration
    isDST = isNewfoundlandDST();
    configTime((TIME_ZONE_OFFSET * 3600), (isDST ? 3600 : 0), "pool.ntp.org");
    Serial.print("Initial DST status: ");
    Serial.println(isDST ? "ON" : "OFF");
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

// Update the getCurrentFormattedTime() function
String getCurrentFormattedTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "00";
  }
  
  // // Add debug print
  // Serial.print("Raw hour from system: ");
  // Serial.println(timeinfo.tm_hour);
  
  char timeString[3];
  strftime(timeString, 3, "%H", &timeinfo);

  // Log the time
  Serial.print("Current time: ");
  Serial.println(timeString);
  
  return String(timeString);
}

void displayRelayStatuses() {
    Serial.println("\n=== Relay Statuses ===");
    Serial.println("Relay 1 (Water Heater): " + relay1.getStatus() + 
                  " [Current: " + String(currentWaterTemp) + "°C, Target: " + String(targetWaterTemp) + "°C]");
    Serial.println("Relay 2 (Room Heater): " + relay2.getStatus() + 
                  " [Current: " + String(currentHeaterTemp) + "°C, Target: " + String(targetHeaterTemp) + "°C]");
    Serial.println("Relay 3 (Water Pump): " + relay3.getStatus() + 
                  " [On: " + String(onInterval) + "min, Off: " + String(offInterval) + "min]");
    Serial.println("Relay 4 (Lights): " + relay4.getStatus() + 
                  " [On: " + String(onHour) + ":00, Off: " + String(offHour) + ":00]");
    Serial.println("Current Time: " + currentTime + ":00");
    Serial.println("==================\n");
}

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

  // Connect to WiFi first
  connectToWiFi();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  // WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  // WiFi.printDiag(Serial); // Uncomment to verify channel change after

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Remove or comment out this line since we're now configuring time in connectToWiFi()
  // configTime(0, 0, "pool.ntp.org");
 
}

/*****************************************\
 *   MAIN PROGRAM LOOP
\*****************************************/
void loop()
{
  static unsigned long lastDSTCheck = 0;
  unsigned long currentMillis = millis();
  
  // Check DST status every hour
  if (currentMillis - lastDSTCheck >= 3600000) {
      updateTimeConfig();
      lastDSTCheck = currentMillis;
  }

  // Get current time at the start of each loop
  currentTime = getCurrentFormattedTime();

  // Relay Check
  if (currentMillis - previousMillis >= interval || previousMillis == 0)
  {
    // Message
    Serial.println("Checking Relays");

    relay1.setRelayforTemp(currentWaterTemp, targetWaterTemp);    // Water Temperature
    relay2.setRelayforTemp(currentHeaterTemp, targetHeaterTemp);  // Room Temperature
    relay3.setRelayForTimedIntervals(onInterval, offInterval);    // Water Pump
    relay4.setRelayForSchedule(onHour, offHour, currentTime);     // Lights

    // Display relay statuses
    displayRelayStatuses();

    previousMillis = currentMillis;
  }

  // Add a delay to avoid rapid toggling
  delay(5000);
}