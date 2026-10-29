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

    // Network state tracking
    String lastConnectedSSID = "";
    String lastConnectedPassword = "";
    bool hasStoredNetworkConfig = false;
    unsigned long lastReconnectAttempt = 0;
    const unsigned long RECONNECT_COOLDOWN = 30000; // 30 seconds between retry attempts

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

    // Helper functions for NVS key checking with different data types
    uint64_t checkNVSKeyULong64(const char *keyName, uint64_t defaultValue, const char *settingName);
    unsigned long checkNVSKeyULong(const char *keyName, unsigned long defaultValue, const char *settingName);
    String checkNVSKeyString(const char *keyName, const String &defaultValue, const char *settingName);
    bool checkNVSKeyBool(const char *keyName, bool defaultValue, const char *settingName);
    bool hasNVSSettingChanged(const char *file, String keyName, uint32_t &newValue);
    bool hasBoolNVSSettingChanged(const char *file, String keyName, bool newValue);

    String formatTimestamp(unsigned long timestamp);
    String getStatusText(int status);
    String getStatusColor(int status);

public:
    WiFiCredentials loadWiFiCredentials(); // New function
    DeviceSettings loadDeviceSettings();   // New function for loading device settings
    void saveDeviceSettings(const DeviceSettings &settings); // Save device settings to NVS

    // WiFi and network methods
    void setupWiFi(WiFiCredentials credentials, String idCode, bool apON); // Modified to accept credentials
    bool connectToNetwork(String ssid, String password);
    bool reconnectToNetwork(int maxRetries = 3); // New reconnection method with retries
    void disconnectWiFi();                       // New method to properly disconnect WiFi before sleep
    void saveNetworkConfig(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2);
    bool loadNetworkConfig(IPAddress &ip, IPAddress &gateway, IPAddress &subnet, IPAddress &dns1, IPAddress &dns2);
    void startWebServer(); // New function to start web server in station mode

    // AP and Web Server methods
    void setupAP(String idCode);
    void handleClientRequests();
    void handleClientRequestsWithSensorData(const LatestReadings &readings); // Updated method for sensor data
    void sendWiFiConfigPage(WiFiClient &client);

    // Configuration processing methods
    void processWiFiConfig(WiFiClient &client, String request);
    void saveWiFiCredentials(String ssid, String password);

    // Helpers for mode and time
    bool isAPMode();
    static unsigned long getTime();
    static unsigned long getRTCTime(); // Add this new method declaration
    static bool retryNTPSync();        // New function for periodic NTP retry attempts
    String getCurrentTimeString(const char* timezone = "UTC");     // Returns current time as HH:MM:SS format in specified timezone

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
