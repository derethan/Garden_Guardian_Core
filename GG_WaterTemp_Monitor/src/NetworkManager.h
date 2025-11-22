#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "TemperatureSensor.h"

// Network configuration constants
#define MAX_NETWORKS 20
#define AP_SSID "GG_WaterTemp_Monitor"
#define AP_PASSWORD "ggmonitor123"
#define DNS_PORT 53
#define WEB_SERVER_PORT 80
#define CONNECTION_TIMEOUT 10000
#define RECONNECT_INTERVAL 30000

// Network information structure
struct NetworkInfo {
  String ssid;
  int32_t rssi;
  wifi_auth_mode_t encryptionType;
  bool isOpen;
};

// WiFi credentials structure
struct WiFiCredentials {
  String ssid;
  String password;
  bool isValid;
};

class NetworkManager {
private:
  AsyncWebServer* server;
  DNSServer* dnsServer;
  Preferences preferences;
  TemperatureSensorManager* tempSensorManager;
  
  // Network management
  NetworkInfo availableNetworks[MAX_NETWORKS];
  int networkCount;
  WiFiCredentials savedCredentials;
  bool isAPMode;
  bool isConnected;
  unsigned long lastConnectionAttempt;
  unsigned long lastNetworkScan;
  
  // Web server routes
  void setupWebServerRoutes();
  void handleRoot(AsyncWebServerRequest* request);
  void handleDashboard(AsyncWebServerRequest* request);
  void handleWiFiConfig(AsyncWebServerRequest* request);
  void handleNetworkScan(AsyncWebServerRequest* request);
  void handleSaveWiFi(AsyncWebServerRequest* request);
  void handleGetTemperatures(AsyncWebServerRequest* request);
  void handleRenameSensor(AsyncWebServerRequest* request);
  void handleNotFound(AsyncWebServerRequest* request);
  
  // HTML templates
  String getConfigPageHTML();
  String getDashboardHTML();
  String getNetworkOptionsHTML();
  
  // Network operations
  void scanNetworks();
  void loadCredentialsFromNVS();
  void saveCredentialsToNVS(const String& ssid, const String& password);
  void clearCredentialsFromNVS();
  bool connectToWiFi();
  void startAPMode();
  void stopAPMode();
  void startWebServer();
  void setupCaptivePortal();
  
  // Utility functions
  String getEncryptionTypeString(wifi_auth_mode_t encryptionType);
  String formatRSSI(int32_t rssi);
  String getWiFiStatusString();

public:
  // Constructor
  NetworkManager(TemperatureSensorManager* tempManager);
  
  // Destructor
  ~NetworkManager();
  
  // Public methods
  void begin();
  void update();
  bool isWiFiConnected();
  bool isInAPMode();
  String getIPAddress();
  String getAPIPAddress();
  int getConnectedNetworkCount();
  void forceAPMode();
  void resetNetworkSettings();
  void printNetworkStatus();
  void refreshNetworkScan();
};

#endif // NETWORK_MANAGER_H