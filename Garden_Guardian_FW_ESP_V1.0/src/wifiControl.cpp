
#include "wifiControl.h"

// Constructor
WifiControl::WifiControl(const char *ssid, const char *pass) : ssid(ssid), pass(pass), status(WL_IDLE_STATUS)
{
}

void WifiControl::connect()
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