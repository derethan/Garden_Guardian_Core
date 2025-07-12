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

// Include dataProvider for sensor data structures
#include "dataProvider.h"

// Include latest readings structure
#include "latestReadings.h"

struct WiFiCredentials
{
    String ssid;
    String password;
    bool valid; // Indicates if credentials were successfully read
};

struct DeviceSettings
{
    uint64_t sleepDuration = 15ULL * 1000000ULL;   // 15 seconds in microseconds
    unsigned long sensorReadInterval = 30000;      // 30 seconds between readings
    unsigned long sensorStabilizationTime = 60000; // 1 minute stabilization
    String deviceID = DEVICE_ID;                   // Default device ID
    String idCode = IDCODE;                        // Unique device identifier
    bool ntpRetryEnabled = true;                   // Enable automatic NTP retry attempts
    unsigned long ntpRetryInterval = 3600000;      // 1 hour between NTP retry attempts (milliseconds)
    bool httpPublishEnabled = true;                // Enable HTTP data publishing
    unsigned long httpPublishInterval = 300000;    // 5 minutes between HTTP publications (milliseconds)
    bool valid = false;                            // Indicates if settings were loaded successfully
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
    bool webServerStarted = false; // Track if web server is running

    void saveWiFiCredentials(String ssid, String password);
    void saveDeviceSettings(const DeviceSettings &settings);
    void scanNetworks();
    void printNetworkInfo();

    void sendHTTPHeader(WiFiClient &client, int statusCode = 200);
    void sendHTMLHeader(WiFiClient &client, const char *title);
    void sendPageHeader(WiFiClient &client);
    void sendPageFooter(WiFiClient &client); // Sensor data web server methods
    void sendSensorDataPage(WiFiClient &client, const LatestReadings &readings);
    void sendSensorDataJSON(WiFiClient &client, const LatestReadings &readings);
    void sendAdvancedConfigPage(WiFiClient &client, const DeviceSettings &settings);
    void processAdvancedConfig(WiFiClient &client, String request);
    String formatTimestamp(unsigned long timestamp);
    String getStatusText(int status);
    String getStatusColor(int status);

public:
    WiFiCredentials loadWiFiCredentials(); // New function
    DeviceSettings loadDeviceSettings();   // New function for loading device settings

    void setupWiFi(WiFiCredentials credentials, String idCode, bool apON); // Modified to accept credentials
    bool connectToNetwork(String ssid, String password);
    void startWebServer(); // New function to start web server in station mode

    void setupAP(String idCode);
    void handleClientRequests();
    void handleClientRequestsWithSensorData(const LatestReadings &readings); // Updated method for sensor data
    void sendWiFiConfigPage(WiFiClient &client);
    void processWiFiConfig(WiFiClient &client, String request);
    bool isAPMode();
    static unsigned long getTime();
    static unsigned long getRTCTime(); // Add this new method declaration
    static bool retryNTPSync();        // New function for periodic NTP retry attempts
                                       // HTTP Publishing functions
    bool publishSensorData(const SensorDataManager &sensorData, const String &deviceID);
    bool sendSensorDataHTTP(const sensorData &data, const String &deviceID);
    bool testServerConnection(const String &deviceID);

    bool isConnected();
    void processDNSRequests()
    {
        dnsServer.processNextRequest();
    }
};

#endif // NETWORKCONNECTIONS_H
