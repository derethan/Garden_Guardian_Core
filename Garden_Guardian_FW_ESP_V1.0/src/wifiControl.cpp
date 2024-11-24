
#include "wifiControl.h"

// Constructor
WifiControl::WifiControl(const char *ssid, const char *pass) : ssid(ssid), pass(pass), status(WL_IDLE_STATUS)
{
}

void WifiControl::connect()
{
    if (WiFi.status() == WL_IDLE_STATUS)
    {
        Serial.println("Communication with WiFi module failed!");
        while (true)
            ;
    }

    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        delay(2000);
    }

    Serial.println("You're connected to the network");
    printStatus();
}

void WifiControl::printStatus()
{
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