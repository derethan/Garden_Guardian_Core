#ifndef NETWORKCONNECTIONS_H
#define NETWORKCONNECTIONS_H

#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <time.h>
#include <sys/time.h>

#include <DNSServer.h>

// SD card library
#include <Preferences.h> // Required for NVS

// credentials and definitions
#include "secrets.h"

struct WiFiCredentials
{
    String ssid;
    String password;
    bool valid; // Indicates if credentials were successfully read
};

extern int wifiStatus;
extern int status;

class NetworkConnections
{
private:
    String urlDecode(String input);
    DNSServer dnsServer;
    String apSSID;
    Preferences preferences;

    void saveWiFiCredentials(String ssid, String password);
    void scanNetworks();
    void printNetworkInfo();

    void sendHTTPHeader(WiFiClient &client, int statusCode = 200);
    void sendHTMLHeader(WiFiClient &client, const char *title);
    void sendPageHeader(WiFiClient &client);
    void sendPageFooter(WiFiClient &client);

public:
    WiFiCredentials loadWiFiCredentials(); // New function

    void setupWiFi(WiFiCredentials credentials, String idCode, bool apON); // Modified to accept credentials
    bool connectToNetwork(String ssid, String password);

    void setupAP(String idCode);

    void handleClientRequests();
    void sendWiFiConfigPage(WiFiClient &client);
    void processWiFiConfig(WiFiClient &client, String request);
    bool isAPMode();
    static unsigned long getTime();
    static unsigned long getRTCTime(); // Add this new method declaration

    bool isConnected();
    void processDNSRequests()
    {
        dnsServer.processNextRequest();
    }
};

#endif // NETWORKCONNECTIONS_H
