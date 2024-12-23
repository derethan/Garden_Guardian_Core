
#ifndef WIFICONTROL_H
#define WIFICONTROL_H

#include <Arduino.h>
#include <WiFi.h>
// #include <WiFiClientSecure.h>

class WifiControl
{
public:
    // ssl wifi client
    WifiControl(const char *ssid, const char *pass);
    void connect();
    void printStatus();
    void modemStatus(int status);

private:
    const char *ssid;
    const char *pass;
    int status;
};

#endif // WIFICONTROL_H