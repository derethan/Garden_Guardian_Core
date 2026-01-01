#include "wifiControl.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

// Create a struct_message called myData
struct_message myData;
uint8_t broadcastAddress[6] = {0xCC, 0xDB, 0xA7, 0x32, 0x07, 0xBC};

// Constructor
WifiControl::WifiControl() : status(WL_IDLE_STATUS) {}

/*************************************************
 *     Wi-Fi Functions
 ************************************************/

void WifiControl::setMode(WiFiMode_t mode)
{
    WiFi.mode(mode);
}

/*************************************************
 *     ESP-NOW Functions
 ************************************************/

void WifiControl::initESPNow()
{
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register callback function
    esp_now_register_send_cb(onDataSent);
}

void WifiControl::addPeer(uint8_t *peerAddress, int channel)
{
    // Register peer
    memcpy(peerInfo.peer_addr, peerAddress, 6);
    // peerInfo.channel = channel;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Error adding peer");
        return;
    }
}

bool WifiControl::sendData(struct_message *message, uint8_t *peerAddress)
{
    esp_err_t result = esp_now_send(peerAddress, (uint8_t *)message, sizeof(struct_message));
    return result == ESP_OK;
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

/*************************************************
 *    Command Processing - Relay Control
 ************************************************/

void WifiControl::connect(const char *ssid, const char *pass) // Update method definition
{
    WiFi.begin(ssid, pass);

    Serial.println("Attempting to connect to Wi-Fi...");

    Serial.print("Connecting...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("Connected to Wi-Fi");
    printStatus();
}

void WifiControl::printStatus()
{
    int status = WiFi.status();

    modemStatus(status);

    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    long rssi = WiFi.RSSI();
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void WifiControl::modemStatus(int status)
{
    switch (status)
    {
    case WL_NO_SHIELD:
        Serial.println("No Wi-Fi shield detected");
        break;
    case WL_IDLE_STATUS:
        Serial.println("Idle status");
        break;
    case WL_NO_SSID_AVAIL:
        Serial.println("No SSID available");
        break;
    case WL_SCAN_COMPLETED:
        Serial.println("Scan completed");
        break;
    case WL_CONNECTED:
        Serial.println("Connected to Wi-Fi");
        break;
    case WL_CONNECT_FAILED:
        Serial.println("Connection failed");
        break;
    case WL_CONNECTION_LOST:
        Serial.println("Connection lost");
        break;
    case WL_DISCONNECTED:
        Serial.println("Disconnected");
        break;
    }
}

time_t WifiControl::getCurrentTime()
{
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // NTP server, offset in seconds, update interval in ms
    timeClient.begin();
    while (!timeClient.update())
    {
        timeClient.forceUpdate();
    }
    return timeClient.getEpochTime();
}

// Add this new function
String WifiControl::getFormattedTime()
{
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
    timeClient.begin();

    // set timezone, daylight offset, daylight saving offset
    timeClient.setTimeOffset(-12600); // UTC - 3:30 for Newfoundland Time
    
    while (!timeClient.update())
    {
        timeClient.forceUpdate();
    }
    
    // Get hours, minutes and seconds
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    
    char timeString[9];  // HH:mm:ss\0
    snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d",
        ptm->tm_hour,
        ptm->tm_min,
        ptm->tm_sec);

        // Log the time
    Serial.print("Current Time: ");
    Serial.println(timeString);
        
    return String(timeString);
}