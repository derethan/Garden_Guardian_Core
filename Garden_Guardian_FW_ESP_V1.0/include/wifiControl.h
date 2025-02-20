#ifndef WIFICONTROL_H
#define WIFICONTROL_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
// #include <WiFiClientSecure.h>

extern uint8_t broadcastAddress[6];

// Must match the receiver structure
typedef struct struct_message
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

} struct_message;

// Callback when data is sent
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

class WifiControl
{

public:
    WifiControl();
    void setMode(WiFiMode_t mode);
    void initESPNow();
    void addPeer(uint8_t *peerAddress, int channel = 0);
    bool sendData(struct_message *message, uint8_t *peerAddress);
    void connect(const char *ssid, const char *pass);
    time_t getCurrentTime();
    String getFormattedTime();

private:
    int status;

    esp_now_peer_info_t peerInfo; // Create a struct to hold the peer information
    void modemStatus(int status);
    void printStatus();
};

#endif // WIFICONTROL_H