
#ifndef WIFICONTROL_H
#define WIFICONTROL_H

#include <Arduino.h>
#include <WiFi.h>
// #include <WiFiClientSecure.h>


class WifiControl {
public:
    WifiControl(const char* ssid, const char* pass);
    void connect();
    void printStatus();

private:
    const char* ssid;
    const char* pass;
    int status;
};

#endif // WIFICONTROL_H