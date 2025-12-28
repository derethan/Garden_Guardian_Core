/**
 * @file networkConnections.cpp
 * @brief Network connection management for WiFi, NTP, HTTP, and device configuration
 *
 * Handles all network-related functionality including:
 * - WiFi station and access point modes
 * - NTP time synchronization with RTC fallback
 * - HTTP client for data publishing
 * - Device settings storage and retrieval from NVS
 * - Network reconnection logic
 */

#include "NetworkConnections.h"
#include <Preferences.h> // Required for NVS
#include <esp_task_wdt.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "dataProvider.h" // Include for sensor data access
#include "server.h"       // Include for server configuration
#include "base/sysLogs.h" // Include for logging functions

// Global Variables
int wifiStatus = WL_IDLE_STATUS; // WiFi radio status
int status = WL_IDLE_STATUS;     // WiFi connection status
bool apMode = false;             // Tracks if the device is in AP mode
String availableNetworks = "";   // Stores the Wi-Fi list for later use
char ssid[25] = "";
char password[25] = "";

WiFiServer server(80); // Create a server object
static WiFiUDP ntpUDP; // Used for NTP requests

Preferences preferences;

/**
 * @brief Setup WiFi connection in station mode, AP mode, or dual mode
 * @param credentials WiFi credentials structure containing SSID and password
 * @param idCode Unique device identifier code for AP name
 * @param apOn If true, run in dual mode (AP + Station), otherwise station only
 *
 * Configures WiFi mode based on parameters. In dual mode, both AP and station
 * are active. In station-only mode, falls back to AP if connection fails.
 */
void NetworkConnections::setupWiFi(WiFiCredentials credentials, String idCode, bool apOn)
{
    // If ApOn is true, then the AP mode is always on, use Wifi Mode for both Station and Ap
    if (apOn)
    {
        SysLogs::logInfo("NETWORK", "Configuring dual mode (AP + Station)");
        WiFi.mode(WIFI_AP_STA);

        // Setup AP first
        setupAP(idCode);
        delay(1000); // Delay to allow AP setup to complete

        // Then try to connect to WiFi if credentials are valid
        if (credentials.valid)
        {
            if (connectToNetwork(credentials.ssid, credentials.password))
            {
                printNetworkInfo();
            }
            else
            {
                SysLogs::logWarning("Failed to connect to WiFi, but AP remains active");
            }
        }
    }
    else
    {
        SysLogs::logInfo("NETWORK", "Configuring station mode only");
        WiFi.mode(WIFI_STA);
        delay(1000);

        // Try connecting if credentials are valid
        if (credentials.valid)
        {
            if (connectToNetwork(credentials.ssid, credentials.password))
            {
                printNetworkInfo();
            }
            else
            {
                setupAP(idCode); // Fall back to AP mode if connection fails
            }
        }
        else
        {
            setupAP(idCode); // Start AP mode if credentials are missing or invalid
        }
    }
}

//------------------------------------------------------------------------------
// Core Network Functions
//------------------------------------------------------------------------------

/**
 * @brief Load WiFi credentials from NVS (Non-Volatile Storage)
 * @return WiFiCredentials structure containing SSID, password, and validity flag
 *
 * Attempts to load WiFi credentials from ESP32's NVS storage. Returns a structure
 * with the valid flag set to false if credentials are missing or empty.
 */
WiFiCredentials NetworkConnections::loadWiFiCredentials()
{
    WiFiCredentials credentials;
    credentials.valid = false; // Default to invalid

    // Attempt to load credentials from NVS
    SysLogs::logInfo("NETWORK", "Loading Credentials from NVS storage...");
    preferences.begin("wifi", true); // Read-only mode
    credentials.ssid = preferences.getString("ssid", "");
    credentials.password = preferences.getString("password", "");
    preferences.end();

    if (credentials.ssid.length() > 0 && credentials.password.length() > 0)
    {
        credentials.valid = true;
        SysLogs::logSuccess("NETWORK", "Wi-Fi credentials loaded successfully from NVS.");
        SysLogs::logInfo("NETWORK", "SSID: " + credentials.ssid);
    }
    else
    {
        SysLogs::logWarning("No Wi-Fi credentials found in NVS. Starting AP mode.");
    }

    return credentials;
}

bool NetworkConnections::connectToNetwork(String ssid, String password)
{
    // Define timeout period (20 seconds)
    const unsigned long TIMEOUT = 20000; // 20 seconds in milliseconds
    unsigned long startAttempt = millis();

    SysLogs::print("Attempting to connect to SSID: ");
    SysLogs::println(ssid);

    // Try to load and apply saved network configuration first
    IPAddress savedIP, savedGateway, savedSubnet, savedDNS1, savedDNS2;
    if (loadNetworkConfig(savedIP, savedGateway, savedSubnet, savedDNS1, savedDNS2))
    {
        SysLogs::logInfo("NETWORK", " (using saved IP configuration)");
        if (!WiFi.config(savedIP, savedGateway, savedSubnet, savedDNS1, savedDNS2))
        {
            SysLogs::logInfo("NETWORK", "Failed to configure static IP, falling back to DHCP");
        }
    }
    else
    {
        SysLogs::logInfo("NETWORK", " (using DHCP)");
    }

    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection or timeout
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < TIMEOUT)
    {
        SysLogs::print(".");
        delay(500); // Check every 500ms

        // Print progress every 3 seconds
        if ((millis() - startAttempt) % 3000 == 0)
        {
            SysLogs::println();
            SysLogs::logInfo("NETWORK", "Still trying to connect... (" + String((millis() - startAttempt) / 1000.0, 1) + " seconds elapsed)");
        }

        // Feed watchdog if available
        esp_task_wdt_reset();
    }
    SysLogs::println();

    if (WiFi.status() == WL_CONNECTED)
    {

        // Store successful connection details
        lastConnectedSSID = ssid;
        lastConnectedPassword = password;

        // Save network configuration for future use
        saveNetworkConfig(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), WiFi.dnsIP(0), WiFi.dnsIP(1));

        delay(1000); // Delay to allow connection to stabilize
        return true; // Successfully connected
    }
    else
    {
        SysLogs::logInfo("NETWORK", "WiFi connection failed.");
        WiFi.disconnect(); // Clean up the failed connection attempt
        return false;      // Connection failed
    }
}

void NetworkConnections::startWebServer()
{
    // Check if web server is already running
    if (webServerStarted)
    {
        SysLogs::logInfo("NETWORK", "[WEB] Web server is already running");
        return;
    }

    SysLogs::logInfo("NETWORK", "[WEB] Starting web server on port 80...");

    // Start the web server
    server.begin();
    webServerStarted = true;

    SysLogs::logInfo("NETWORK", "[WEB] Web server started successfully");
    SysLogs::print("[WEB] Access the device dashboard at: http://");
    SysLogs::println(WiFi.localIP().toString());
    SysLogs::logInfo("NETWORK", "[WEB] Available endpoints:");
    SysLogs::logInfo("NETWORK", "[WEB]   / or /index    - Sensor data dashboard");
    SysLogs::logInfo("NETWORK", "[WEB]   /data          - JSON API endpoint");
    SysLogs::logInfo("NETWORK", "[WEB]   /config        - WiFi configuration");
    SysLogs::logInfo("NETWORK", "[WEB]   /advanced      - Advanced device settings");
}

unsigned long NetworkConnections::getTime()
{
    // Configuration for NTP synchronization
    const char *ntpServers[] = {
        "pool.ntp.org",
        "time.nist.gov",
        "time.google.com",
        "time.cloudflare.com"};
    const int numServers = sizeof(ntpServers) / sizeof(ntpServers[0]);
    const unsigned long NTP_TIMEOUT = 15000;         // 15 seconds timeout per attempt
    const int MAX_RETRIES = 3;                       // Maximum number of retry attempts
    const unsigned long MIN_VALID_TIME = 1577836800; // Jan 1, 2020 timestamp
    const unsigned long RETRY_DELAY_BASE = 2000;     // Base delay between retries (2 seconds)

    SysLogs::logInfo("NETWORK", "Starting NTP time synchronization...");

    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++)
    {
        SysLogs::logInfo("NETWORK", "NTP sync attempt " + String(attempt) + "/" + String(MAX_RETRIES) + "");

        // Try each NTP server for this attempt
        for (int serverIndex = 0; serverIndex < numServers; serverIndex++)
        {
            const char *currentServer = ntpServers[serverIndex];
            SysLogs::logInfo("NETWORK", "Trying NTP server: " + String(currentServer) + "");

            // Configure NTP time synchronization with UTC (no timezone offset)
            configTime(0, 0, currentServer);

            // Wait for NTP synchronization with timeout
            unsigned long startTime = millis();
            struct tm timeinfo;
            bool syncSuccess = false;

            SysLogs::print("Waiting for sync");
            while ((millis() - startTime < NTP_TIMEOUT))
            {
                if (getLocalTime(&timeinfo))
                {
                    syncSuccess = true;
                    break;
                }
                SysLogs::print(".");
                delay(500);

                // Feed the watchdog timer to prevent reset during long sync attempts
                esp_task_wdt_reset();
            }
            SysLogs::println();

            if (!syncSuccess)
            {
                SysLogs::logWarning("NTP server " + String(currentServer) + " timed out after " + String(NTP_TIMEOUT) + " ms");
                continue; // Try next server
            }

            // Get the synchronized time
            struct timeval tv;
            if (gettimeofday(&tv, NULL) != 0)
            {
                SysLogs::logError("Failed to obtain time from " + String(currentServer) + " after sync");
                continue; // Try next server
            }

            // Verify we got a reasonable time (after year 2020)
            if (tv.tv_sec < MIN_VALID_TIME)
            {
                SysLogs::logWarning("NTP server " + String(currentServer) + " returned invalid time: " + String((unsigned long)tv.tv_sec) + " (before 2020)");
                continue; // Try next server
            }

            // Success! Log the results
            SysLogs::logSuccess("NETWORK", "NTP synchronization successful with " + String(currentServer) + "!");
            SysLogs::logInfo("NETWORK", "Sync completed in " + String(millis() - startTime) + " ms");
            SysLogs::logInfo("NETWORK", "Current time (Unix timestamp): " + String((unsigned long)tv.tv_sec));
            SysLogs::logInfo("NETWORK", "Current time (Human-readable): " + String(ctime(&tv.tv_sec)));

            // The ESP32's RTC is automatically updated by configTime() and getLocalTime()
            return (unsigned long)tv.tv_sec;
        }

        // If we reach here, all servers failed for this attempt
        if (attempt < MAX_RETRIES)
        {
            // Calculate exponential backoff delay: base * 2^(attempt-1)
            unsigned long retryDelay = RETRY_DELAY_BASE * (1 << (attempt - 1));
            SysLogs::logInfo("NETWORK", "All NTP servers failed for attempt " + String(attempt) + ". Retrying in " + String(retryDelay) + " ms...");

            // Wait with watchdog feeding
            unsigned long delayStart = millis();
            while (millis() - delayStart < retryDelay)
            {
                delay(100);
                esp_task_wdt_reset();
            }
        }
    }

    // All attempts failed
    SysLogs::logInfo("NETWORK", "ERROR: NTP synchronization failed after all retry attempts!");
    SysLogs::logInfo("NETWORK", "Possible causes:");
    SysLogs::logInfo("NETWORK", "  - No internet connection");
    SysLogs::logInfo("NETWORK", "  - DNS resolution failure");
    SysLogs::logInfo("NETWORK", "  - NTP servers unreachable");
    SysLogs::logInfo("NETWORK", "  - Firewall blocking NTP traffic");
    SysLogs::logInfo("NETWORK", "Device will continue with RTC time if available.");

    return 0; // Return 0 to indicate failure
}

unsigned long NetworkConnections::getRTCTime()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        SysLogs::logInfo("NETWORK", "ERROR: Failed to read time from RTC");
        return 0; // Return 0 if RTC read fails
    }

    // Check if time is unreasonable (before 2024)
    // Unix timestamp for Jan 1, 2024: 1704067200
    const unsigned long MIN_VALID_RTC_TIME = 1704067200;
    if (tv.tv_sec < MIN_VALID_RTC_TIME)
    {
        SysLogs::logWarning("RTC time appears invalid (" + String((unsigned long)tv.tv_sec) + " - before 2024)");
        SysLogs::logInfo("NETWORK", "This may indicate the RTC was never synchronized or has lost power");
        return 0;
    }

    // Log successful RTC read
    // SysLogs::logInfo("NETWORK", "RTC time read successfully: %lu (%s)", (unsigned long)tv.tv_sec, ctime(&tv.tv_sec));

    return (unsigned long)tv.tv_sec;
}

bool NetworkConnections::reconnectToNetwork(int maxRetries)
{
    // Check cooldown period
    if (millis() - lastReconnectAttempt < RECONNECT_COOLDOWN)
    {
        return false;
    }

    lastReconnectAttempt = millis();

    SysLogs::logInfo("NETWORK", "[RECONNECT] Starting WiFi reconnection process...");

    // If we have stored credentials, try to reconnect
    if (lastConnectedSSID.length() > 0 && lastConnectedPassword.length() > 0)
    {
        for (int attempt = 1; attempt <= maxRetries; attempt++)
        {
            SysLogs::logInfo("RECONNECT", "Attempt " + String(attempt) + "/" + String(maxRetries) + " to reconnect to " + lastConnectedSSID);

            // Ensure WiFi is in station mode
            WiFi.mode(WIFI_STA);
            delay(100);

            if (connectToNetwork(lastConnectedSSID, lastConnectedPassword))
            {
                SysLogs::logInfo("NETWORK", "[RECONNECT] Successfully reconnected!");
                return true;
            }

            // Progressive delay between attempts
            if (attempt < maxRetries)
            {
                unsigned long delayTime = attempt * 2000; // 2s, 4s, 6s...
                SysLogs::logInfo("NETWORK", "[RECONNECT] Waiting " + String(delayTime) + " ms before next attempt");
                delay(delayTime);
            }
        }

        SysLogs::logInfo("NETWORK", "[RECONNECT] All reconnection attempts failed");
    }
    else
    {
        SysLogs::logInfo("NETWORK", "[RECONNECT] No stored credentials available");
        // Try loading from NVS
        WiFiCredentials credentials = loadWiFiCredentials();
        if (credentials.valid)
        {
            return connectToNetwork(credentials.ssid, credentials.password);
        }
    }

    return false;
}

void NetworkConnections::disconnectWiFi()
{
    SysLogs::logInfo("NETWORK", "[WIFI] Disconnecting WiFi for sleep...");
    WiFi.disconnect(true); // true = turn off WiFi radio
    WiFi.mode(WIFI_OFF);
    delay(100);
    SysLogs::logInfo("NETWORK", "[WIFI] WiFi disconnected and radio turned off");
}

bool NetworkConnections::hasNVSSettingChanged(const char *file, String keyName, uint32_t &newValue)
{
    bool changesMade = false;
    preferences.begin(file, false); // Read-write mode

    uint32_t currentValue = preferences.getUInt(keyName.c_str(), 0);
    if (currentValue != newValue)
    {
        preferences.putUInt(keyName.c_str(), newValue);
        SysLogs::logInfo("NETWORK", "[NETWORK] NVS key '" + keyName + "' updated to: " + String(newValue));
        changesMade = true;
    }
    else
    {
        // SysLogs::logInfo("NETWORK", "[NETWORK] NVS key '" + keyName + "' unchanged, no write needed");
    }

    preferences.end();
    return changesMade;
}

bool NetworkConnections::hasBoolNVSSettingChanged(const char *file, String keyName, bool newValue)
{
    bool changesMade = false;
    preferences.begin(file, false); // Read-write mode

    bool currentValue = preferences.getBool(keyName.c_str(), false);
    if (currentValue != newValue)
    {
        preferences.putBool(keyName.c_str(), newValue);
        SysLogs::logInfo("NETWORK", "[NETWORK] NVS key '" + keyName + "' updated to: " + String(newValue ? "true" : "false"));
        changesMade = true;
    }
    else
    {
        SysLogs::logInfo("NETWORK", "[NETWORK] NVS key '" + keyName + "' unchanged, no write needed");
    }

    preferences.end();
    return changesMade;
}

void NetworkConnections::saveNetworkConfig(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2)
{
    SysLogs::logInfo("NETWORK", "[NETWORK] Checking network configuration for changes...");

    // Convert IPAddress to uint32_t properly
    uint32_t ipValue = (uint32_t)ip;
    uint32_t gatewayValue = (uint32_t)gateway;
    uint32_t subnetValue = (uint32_t)subnet;
    uint32_t dns1Value = (uint32_t)dns1;
    uint32_t dns2Value = (uint32_t)dns2;

    bool currentIP = hasNVSSettingChanged("network", "ip", ipValue);
    bool currentGateway = hasNVSSettingChanged("network", "gateway", gatewayValue);
    bool currentSubnet = hasNVSSettingChanged("network", "subnet", subnetValue);
    bool currentDNS1 = hasNVSSettingChanged("network", "dns1", dns1Value);
    bool currentDNS2 = hasNVSSettingChanged("network", "dns2", dns2Value);
    bool hasConfig = hasBoolNVSSettingChanged("network", "hasConfig", true);

    hasStoredNetworkConfig = true;

    if (currentIP || currentGateway || currentSubnet || currentDNS1 || currentDNS2 || hasConfig)
    {
        SysLogs::logInfo("NETWORK", "Saved updated network config - IP: " + ip.toString() + ", Gateway: " + gateway.toString());
    }
    else
    {
        SysLogs::logInfo("NETWORK", "Network configuration unchanged, no NVS write needed");
    }
}

bool NetworkConnections::loadNetworkConfig(IPAddress &ip, IPAddress &gateway, IPAddress &subnet, IPAddress &dns1, IPAddress &dns2)
{
    preferences.begin("network", true); // Read-only mode

    bool hasConfig = preferences.getBool("hasConfig", false);
    if (!hasConfig)
    {
        preferences.end();
        return false;
    }

    ip = IPAddress(preferences.getUInt("ip", 0));
    gateway = IPAddress(preferences.getUInt("gateway", 0));
    subnet = IPAddress(preferences.getUInt("subnet", 0));
    dns1 = IPAddress(preferences.getUInt("dns1", 0));
    dns2 = IPAddress(preferences.getUInt("dns2", 0));

    preferences.end();

    // Validate that we have reasonable IP addresses
    if (ip[0] == 0 || gateway[0] == 0)
    {
        SysLogs::logInfo("NETWORK", "[NETWORK] Stored network config is invalid");
        return false;
    }

    hasStoredNetworkConfig = true;
    SysLogs::logInfo("NETWORK", "Loaded network config - IP: " + ip.toString() + ", Gateway: " + gateway.toString());
    return true;
}

//------------------------------------------------------------------------------
// Access Point Mode Functions
//------------------------------------------------------------------------------
void NetworkConnections::setupAP(String idCode)
{
    scanNetworks(); // Scan for available networks
    delay(1000);    // Delay to allow scan to complete

    apSSID = AP_SSID + idCode;

    // print the network name (SSID);
    SysLogs::print("Creating access point named: ");
    SysLogs::println(apSSID);

    WiFi.mode(WIFI_AP); // Set ESP32 to AP mode

    // Configure DNS
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);

    // WiFi.softAP(SSID, Password, channel, hidden, max_connections);
    status = WiFi.softAP(apSSID.c_str(), AP_PASS);

    if (!status)
    {
        SysLogs::logInfo("NETWORK", "Creating access point failed");
        return;
    }
    apMode = true;

    dnsServer.start(53, "*", local_IP);
    server.begin();
    webServerStarted = true; // Mark web server as started

    SysLogs::logInfo("NETWORK", "Access Point started successfully");
    SysLogs::print("AP SSID: ");
    SysLogs::println(apSSID);
    SysLogs::print("AP IP Address: ");
    SysLogs::println(WiFi.softAPIP().toString());
}

void NetworkConnections::scanNetworks()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    SysLogs::logInfo("NETWORK", "Scanning for WiFi networks...");
    esp_task_wdt_reset(); // Feed the watchdog in the loop

    int numNetworks = WiFi.scanNetworks();

    SysLogs::print("Number of networks found: ");
    SysLogs::println(String(numNetworks));

    if (numNetworks <= 0)
    {
        SysLogs::logInfo("NETWORK", "No networks found or scan failed.");
        availableNetworks = "<option value=''>No Networks Found</option>";
    }
    else
    {
        SysLogs::logInfo("NETWORK", "Networks found:");
        availableNetworks = ""; // Clear previous results

        for (int i = 0; i < numNetworks; i++)
        {
            esp_task_wdt_reset(); // Feed the watchdog in the loop

            availableNetworks += "<option value='";
            availableNetworks += WiFi.SSID(i);
            availableNetworks += "'>";
            availableNetworks += WiFi.SSID(i);
            availableNetworks += "</option>";

            SysLogs::print(String(i + 1));
            SysLogs::print(": ");
            SysLogs::print(WiFi.SSID(i));
            SysLogs::print(" (RSSI: ");
            SysLogs::print(String(WiFi.RSSI(i)));
            SysLogs::print(" dBm) ");
            SysLogs::print(" [");
            SysLogs::print(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured");
            SysLogs::logInfo("NETWORK", "]");
            delay(10);
        }
    }
}

void NetworkConnections::handleClientRequests()
{
    WiFiClient client = server.available();
    if (client)
    {
        SysLogs::logInfo("NETWORK", "New Client Connected!");
        String request = "";
        unsigned long timeout = millis() + 5000; // 5-second timeout

        // **Read the full request (including headers & body)**
        while (client.connected() && millis() < timeout)
        {
            while (client.available())
            {
                char c = client.read();
                request += c;
            }
        }

        SysLogs::logInfo("NETWORK", "Full HTTP Request:");
        SysLogs::println(request);

        if (request.indexOf("GET /") >= 0)
        {
            sendWiFiConfigPage(client);
        }
        else if (request.indexOf("POST /config") >= 0)
        {
            processWiFiConfig(client, request);
        }

        delay(100);
        client.stop();
        SysLogs::logInfo("NETWORK", "Client Disconnected.");
    }
}

void NetworkConnections::sendWiFiConfigPage(WiFiClient &client)
{
    sendHTTPHeader(client);
    sendHTMLHeader(client, "Device Setup");

    // Additional CSS specific to WiFi config page
    client.println("<style>");
    client.println("label { font-size: 14px; font-weight: bold; display: block; margin-top: 10px; text-align: left; }");
    client.println("select, input { width: 100%; padding: 8px; margin-top: 5px; border: 1px solid #ccc; border-radius: 5px; }");
    client.println("button:disabled { background: #ccc; cursor: not-allowed; }");
    client.println("</style>");

    // JavaScript for input validation
    client.println("<script>");
    client.println("function validateForm() {");
    client.println("  var ssidSelect = document.getElementById('ssid');");
    client.println("  var manualSSID = document.getElementById('manualSSID').value.trim();");
    client.println("  var password = document.getElementById('password').value.trim();");
    client.println("  var submitButton = document.getElementById('submitButton');");
    client.println("  var isSSIDSelected = ssidSelect.value !== '' || manualSSID !== '';");
    client.println("  var isPasswordEntered = password.length > 0;");
    client.println("  submitButton.disabled = !(isSSIDSelected && isPasswordEntered);");
    client.println("}");
    client.println("document.addEventListener('DOMContentLoaded', function() {");
    client.println("  document.getElementById('ssid').addEventListener('change', validateForm);");
    client.println("  document.getElementById('manualSSID').addEventListener('input', validateForm);");
    client.println("  document.getElementById('password').addEventListener('input', validateForm);");
    client.println("});");
    client.println("</script>");

    sendPageHeader(client);

    // Main content
    client.println("<h2>Wi-Fi Setup</h2>");
    client.println("<p>Connect your device to a Wi-Fi network.</p>");
    client.println("<form action='/config' method='POST'>");
    client.println("<label for='ssid'>Select Wi-Fi Network:</label>");
    client.println("<select id='ssid' name='ssid'>");
    client.println("<option value=''>-- Select a Network --</option>");
    client.println(availableNetworks);
    client.println("</select>");
    client.println("<label for='manualSSID'>Or Enter SSID:</label>");
    client.println("<input type='text' id='manualSSID' name='manualSSID' placeholder='Enter network name'>");
    client.println("<label for='password'>Wi-Fi Password:</label>");
    client.println("<input type='password' id='password' name='password' placeholder='Enter password'>");
    client.println("<button type='submit' id='submitButton' disabled>Save & Connect</button>");
    client.println("</form>");

    // Link to advanced settings
    client.println("<div style='margin-top: 20px; text-align: center;'>");
    client.println("<a href='/advanced' style='text-decoration: none; color: #208dbf; font-size: 14px;'>⚙️ Advanced Device Settings</a>");
    client.println("</div>");

    sendPageFooter(client);
}

void NetworkConnections::processWiFiConfig(WiFiClient &client, String request)
{
    SysLogs::logInfo("NETWORK", "Received Wi-Fi Configuration Request:");
    SysLogs::println(request); // Log the full request

    // **Step 1: Extract the POST body correctly**
    int bodyIndex = request.indexOf("\r\n\r\n"); // Find where headers end
    if (bodyIndex == -1)
    {
        SysLogs::logInfo("NETWORK", "Error: Could not locate POST body.");
        return;
    }
    request = request.substring(bodyIndex + 4); // The actual POST data

    SysLogs::logInfo("NETWORK", "Extracted POST Body:");
    SysLogs::println(request); // Should now show 'ssid=MySSID&password=MyPass'

    String ssid = "";
    String password = "";

    // **Step 2: Parse the 'ssid' parameter**
    int ssidStart = request.indexOf("ssid=");
    if (ssidStart != -1)
    {
        ssidStart += 5; // Move past "ssid="
        int ssidEnd = request.indexOf("&", ssidStart);
        if (ssidEnd == -1)
        {
            ssid = request.substring(ssidStart);
        }
        else
        {
            ssid = request.substring(ssidStart, ssidEnd);
        }
        ssid.replace("+", " "); // Replace '+' with spaces
        ssid = urlDecode(ssid);
    }

    // **Check for manualSSID if ssid is empty**
    if (ssid.length() == 0)
    {
        int manualSSIDStart = request.indexOf("manualSSID=");
        if (manualSSIDStart != -1)
        {
            manualSSIDStart += 11; // Move past "manualSSID="
            int manualSSIDEnd = request.indexOf("&", manualSSIDStart);
            if (manualSSIDEnd == -1)
            {
                ssid = request.substring(manualSSIDStart);
            }
            else
            {
                ssid = request.substring(manualSSIDStart, manualSSIDEnd);
            }
            ssid.replace("+", " "); // Replace '+' with spaces
            ssid = urlDecode(ssid);
        }
    }

    // **Step 3: Parse the 'password' parameter**
    int passStart = request.indexOf("password=");
    if (passStart != -1)
    {
        passStart += 9; // Move past "password="
        password = request.substring(passStart);
        password.replace("+", " ");
        password = urlDecode(password);
    }

    SysLogs::print("Extracted SSID: ");
    SysLogs::println(ssid);
    SysLogs::print("Extracted Password: ");
    SysLogs::println(password);

    // **Step 4: Validate and Save**
    if (ssid.length() > 0 && password.length() > 0)
    {
        saveWiFiCredentials(ssid, password);

        sendHTTPHeader(client);
        sendHTMLHeader(client, "Setup Complete");

        // Additional CSS for success page
        client.println("<style>");
        client.println(".success-icon { font-size: 48px; color: #28a745; margin: 20px 0; }");
        client.println("h2 { color: #28a745; }</style>");

        sendPageHeader(client);

        client.println("<div class='success-icon'>Success</div>");
        client.println("<h2>Wi-Fi Configuration Saved!</h2>");
        client.println("<p>Your device will now restart and attempt to connect to the network.</p>");
        client.println("<p>If the connection is successful, this device will no longer broadcast as an access point.</p>");

        sendPageFooter(client);

        delay(3000);
        ESP.restart();
    }
    else
    {
        sendHTTPHeader(client, 400);
        sendHTMLHeader(client, "Setup Error");

        // Additional CSS for error page
        client.println("<style>");
        client.println(".error-icon { font-size: 48px; color: #dc3545; margin: 20px 0; }");
        client.println("h2 { color: #dc3545; }</style>");

        // Add JavaScript for the back button
        client.println("<script>");
        client.println("function goBack() { window.history.back(); }");
        client.println("</script>");

        sendPageHeader(client);

        client.println("<div class='error-icon'>Error</div>");
        client.println("<h2>Error: Invalid Wi-Fi Credentials</h2>");
        client.println("<p>Both SSID and Password are required to connect to a network.</p>");
        client.println("<button onclick='goBack()'>Try Again</button>");

        sendPageFooter(client);
    }
}

//------------------------------------------------------------------------------
// Storage Functions
//------------------------------------------------------------------------------
void NetworkConnections::saveWiFiCredentials(String ssid, String password)
{

    // Save to NVS
    SysLogs::logInfo("NETWORK", "Saving Wi-Fi credentials to NVS...");
    preferences.begin("wifi", false); // Read-write mode
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    SysLogs::logInfo("NETWORK", "Wi-Fi credentials successfully saved to NVS.");
}

void NetworkConnections::saveDeviceSettings(const DeviceSettings &settings)
{
    SysLogs::logInfo("NETWORK", "Saving device settings to NVS...");
    preferences.begin("device", false); // Read-write mode
    preferences.putULong64("sleepDur", settings.sleepDuration);
    preferences.putULong("sensorInt", settings.sensorReadInterval);
    preferences.putULong("stabilTime", settings.sensorStabilizationTime);
    preferences.putString("deviceID", settings.deviceID);
    preferences.putString("idCode", settings.idCode);
    preferences.putBool("ntpRetry", settings.ntpRetryEnabled);
    preferences.putULong("ntpRetryInt", settings.ntpRetryInterval);
    preferences.putBool("httpPubEn", settings.httpPublishEnabled);
    preferences.putULong("httpPubInt", settings.httpPublishInterval);

    preferences.end();
    SysLogs::logInfo("NETWORK", "Device settings successfully saved to NVS.");
}

// Helper functions for NVS key checking with different data types
uint64_t NetworkConnections::checkNVSKeyULong64(const char *keyName, uint64_t defaultValue, const char *settingName)
{
    if (!preferences.isKey(keyName))
    {
        preferences.putULong64(keyName, defaultValue);
        SysLogs::logInfo("NETWORK", "Created default setting: " + String(settingName));
        return defaultValue;
    }
    return preferences.getULong64(keyName, defaultValue);
}

unsigned long NetworkConnections::checkNVSKeyULong(const char *keyName, unsigned long defaultValue, const char *settingName)
{
    if (!preferences.isKey(keyName))
    {
        preferences.putULong(keyName, defaultValue);
        SysLogs::logInfo("NETWORK", "Created default setting: " + String(settingName));
        return defaultValue;
    }
    return preferences.getULong(keyName, defaultValue);
}

String NetworkConnections::checkNVSKeyString(const char *keyName, const String &defaultValue, const char *settingName)
{
    if (!preferences.isKey(keyName))
    {
        preferences.putString(keyName, defaultValue);
        SysLogs::logInfo("NETWORK", "Created default setting: " + String(settingName));
        return defaultValue;
    }
    return preferences.getString(keyName, defaultValue);
}

bool NetworkConnections::checkNVSKeyBool(const char *keyName, bool defaultValue, const char *settingName)
{
    if (!preferences.isKey(keyName))
    {
        preferences.putBool(keyName, defaultValue);
        SysLogs::logInfo("NETWORK", "Created default setting: " + String(settingName));
        return defaultValue;
    }
    return preferences.getBool(keyName, defaultValue);
}

DeviceSettings NetworkConnections::loadDeviceSettings()
{
    DeviceSettings settings;
    SysLogs::logInfo("NETWORK", "Loading device settings from NVS storage...");
    preferences.begin("device", false); // Read-write mode to allow saving defaults

    // Load each setting using helper functions - they handle defaults automatically
    settings.sleepDuration = checkNVSKeyULong64("sleepDur", 15ULL * 1000000ULL, "sleepDur");
    settings.sensorReadInterval = checkNVSKeyULong("sensorInt", 60000, "sensorInt");
    settings.sensorStabilizationTime = checkNVSKeyULong("stabilTime", 15000, "stabilTime");
    settings.deviceID = checkNVSKeyString("deviceID", DEVICE_ID, "deviceID");
    settings.idCode = checkNVSKeyString("idCode", IDCODE, "idCode");
    settings.ntpRetryEnabled = checkNVSKeyBool("ntpRetry", true, "ntpRetry");
    settings.ntpRetryInterval = checkNVSKeyULong("ntpRetryInt", 3600000, "ntpRetryInt");
    settings.httpPublishEnabled = checkNVSKeyBool("httpPubEn", true, "httpPubEn");
    settings.httpPublishInterval = checkNVSKeyULong("httpPubInt", 60000, "httpPubInt");

    preferences.end();

    // Settings are always valid since helper functions ensure they exist
    settings.valid = true;
    SysLogs::logInfo("NETWORK", "Device settings loaded successfully from NVS.");

    // Display loaded settings
    // SysLogs::print("Sleep Duration: ");
    // SysLogs::print(String(settings.sleepDuration / 1000000ULL));
    // SysLogs::logInfo("NETWORK", " seconds");
    // SysLogs::print("Sensor Read Interval: ");
    // SysLogs::print(String(settings.sensorReadInterval / 1000));
    // SysLogs::logInfo("NETWORK", " seconds");
    // SysLogs::print("Stabilization Time: ");
    // SysLogs::print(String(settings.sensorStabilizationTime / 1000));
    // SysLogs::logInfo("NETWORK", " seconds");
    // SysLogs::print("Device ID: ");
    // SysLogs::println(settings.deviceID);
    // SysLogs::print("ID Code: ");
    // SysLogs::println(settings.idCode);
    // SysLogs::print("NTP Retry Enabled: ");
    // SysLogs::println(settings.ntpRetryEnabled ? "Yes" : "No");
    // SysLogs::print("HTTP Publish Enabled: ");
    // SysLogs::println(settings.httpPublishEnabled ? "Yes" : "No");

    return settings;
}

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------
String NetworkConnections::urlDecode(String input)
{
    String decoded = "";
    char temp[] = "00";
    unsigned int len = input.length();

    for (unsigned int i = 0; i < len; i++)
    {
        if (input[i] == '%') // Handle URL encoding (e.g., %20 = space)
        {
            if (i + 2 < len)
            {
                temp[0] = input[i + 1];
                temp[1] = input[i + 2];
                decoded += (char)strtol(temp, NULL, 16);
                i += 2;
            }
        }
        else if (input[i] == '+') // Replace '+' with space
        {
            decoded += ' ';
        }
        else
        {
            decoded += input[i];
        }
    }
    return decoded;
}

// Function to check if the device is in AP mode
bool NetworkConnections::isAPMode()
{
    return apMode;
}

// Function to check if the device is connected to a network
bool NetworkConnections::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

// Add this helper function to keep the code DRY
void NetworkConnections::printNetworkInfo()
{
    SysLogs::logInfo("NETWORK", "---------------Network Info-------------");
    SysLogs::print("SSID: ");
    SysLogs::println(WiFi.SSID());

    IPAddress ip = WiFi.localIP();
    SysLogs::print("IP Address: ");
    SysLogs::println(ip.toString());

    SysLogs::print("Signal Strength (RSSI): ");
    SysLogs::println(String(WiFi.RSSI()) + " dBm");
    SysLogs::logInfo("NETWORK", "----------------------------------------");
}

//------------------------------------------------------------------------------
// Sensor Data Web Server Functions
//------------------------------------------------------------------------------

void NetworkConnections::handleClientRequestsWithSensorData(const LatestReadings &readings)
{
    WiFiClient client = server.available();
    if (client)
    {
        SysLogs::logInfo("NETWORK", "New Client Connected!");
        String request = "";
        unsigned long timeout = millis() + 5000; // 5-second timeout

        // Read the full request (including headers & body)
        while (client.connected() && millis() < timeout)
        {
            if (client.available())
            {
                char c = client.read();
                request += c;

                // Check if we have received the complete request
                if (request.endsWith("\r\n\r\n"))
                {
                    break;
                }
            }
            delay(1);
        }

        SysLogs::logInfo("NETWORK", "Full HTTP Request:");
        SysLogs::println(request); // Handle different routes

        if (request.indexOf("GET / HTTP") >= 0 || request.indexOf("GET /index") >= 0)
        {
            // Main sensor data page
            sendSensorDataPage(client, readings);
        }
        else if (request.indexOf("GET /data") >= 0)
        {
            // JSON data endpoint
            sendSensorDataJSON(client, readings);
        }
        else if (request.indexOf("GET /config") >= 0)
        {
            // WiFi configuration page
            sendWiFiConfigPage(client);
        }
        else if (request.indexOf("GET /advanced") >= 0)
        {
            // Advanced settings page
            DeviceSettings settings = loadDeviceSettings();
            sendAdvancedConfigPage(client, settings);
        }
        else if (request.indexOf("POST /config") >= 0)
        {
            // Process WiFi configuration
            processWiFiConfig(client, request);
        }
        else if (request.indexOf("POST /advanced-config") >= 0)
        {
            // Process advanced configuration
            processAdvancedConfig(client, request);
        }
        else
        {
            // 404 - Not Found
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println("<html><body><h1>404 - Page Not Found</h1></body></html>");
        }

        delay(100);
        client.stop();
        SysLogs::logInfo("NETWORK", "Client Disconnected.");
    }
}

void NetworkConnections::sendSensorDataPage(WiFiClient &client, const LatestReadings &readings)
{
    sendHTTPHeader(client);
    client.println("<html><head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<title>Garden Guardian - Sensor Data</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("<meta http-equiv='refresh' content='30'>"); // Auto-refresh every 30 seconds
    client.println("<style>");

    // Enhanced CSS for sensor data display
    client.println("body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f0f2f5; }");
    client.println(".header { background: linear-gradient(135deg, #208dbf, #1e7ba8); color: white; padding: 20px; text-align: center; border-radius: 10px; margin-bottom: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }");
    client.println(".container { max-width: 1200px; margin: 0 auto; }");
    client.println(".sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 20px; }");
    client.println(".sensor-card { background: white; border-radius: 10px; padding: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); transition: transform 0.2s; }");
    client.println(".sensor-card:hover { transform: translateY(-2px); }");
    client.println(".sensor-title { font-size: 18px; font-weight: bold; margin-bottom: 10px; color: #333; }");
    client.println(".sensor-value { font-size: 32px; font-weight: bold; margin: 10px 0; }");
    client.println(".sensor-unit { font-size: 14px; color: #666; margin-left: 5px; }");
    client.println(".sensor-timestamp { font-size: 12px; color: #888; margin-top: 10px; }");
    client.println(".status-ok { color: #28a745; }");
    client.println(".status-warning { color: #ffc107; }");
    client.println(".status-error { color: #dc3545; }");
    client.println(".info-section { background: white; border-radius: 10px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }");
    client.println(".nav-buttons { text-align: center; margin: 20px 0; }");
    client.println(".nav-buttons a { display: inline-block; background: #208dbf; color: white; text-decoration: none; padding: 10px 20px; margin: 0 10px; border-radius: 5px; transition: background 0.2s; }");
    client.println(".nav-buttons a:hover { background: #1e7ba8; }");
    client.println("</style>");
    client.println("</head><body>");

    client.println("<div class='container'>");
    client.println("<div class='header'>");
    client.println("<h1>Garden Guardian</h1>");
    client.println("<p>Temperature & Humidity Monitor</p>");
    client.println("</div>");
    // Navigation buttons
    client.println("<div class='nav-buttons'>");
    client.println("<a href='/'>Sensor Data</a>");
    client.println("<a href='/data'>JSON Data</a>");
    client.println("<a href='/config'>WiFi Config</a>");
    client.println("<a href='/advanced'>Advanced Settings</a>");
    client.println("</div>"); // System info section
    client.println("<div class='info-section'>");
    client.println("<h3>System Information</h3>");
    client.println("<p><strong>Device Status:</strong> Online</p>");
    client.println("<p><strong>WiFi Network:</strong> ");
    client.println(WiFi.SSID());
    client.println("</p>");
    client.println("<p><strong>IP Address:</strong> ");
    client.println(WiFi.localIP());
    client.println("</p>");
    client.println("<p><strong>Signal Strength:</strong> ");
    client.print(WiFi.RSSI());
    client.println(" dBm</p>");

    // Time synchronization status
    unsigned long currentTime = getRTCTime();
    client.println("<p><strong>Time Status:</strong> ");
    if (currentTime > 0)
    {
        // Check if time is recent (less than 24 hours old in terms of expected sync)
        // We'll consider time valid if RTC is working
        client.println("<span class='status-ok'>Synchronized</span></p>");
        client.println("<p><strong>Current Time:</strong> ");
        client.print(formatTimestamp(currentTime));
    }
    else
    {
        client.println("<span class='status-warning'>Not Synchronized</span></p>");
        client.println("<p><strong>Current Time:</strong> Time not available");
    }
    client.println("</p>");
    client.println("</div>"); // Sensor data grid
    client.println("<div class='sensor-grid'>");

    if (!readings.hasValidData)
    {
        client.println("<div class='sensor-card'>");
        client.println("<div class='sensor-title'>No Data Available</div>");
        client.println("<p>Sensor readings will appear here once data collection begins.</p>");
        client.println("</div>");
    }
    else
    {
        // Temperature Card
        client.println("<div class='sensor-card'>");
        client.println("<div class='sensor-title'>🌡️ Temperature</div>");

        client.print("<div class='sensor-value ");
        client.print(getStatusColor(readings.temperatureStatus));
        client.print("'>");
        if (!isnan(readings.temperature))
        {
            client.print(readings.temperature, 1);
            client.print("<span class='sensor-unit'>°C</span>");
        }
        else
        {
            client.print("--");
        }
        client.println("</div>");

        client.print("<div class='sensor-timestamp'>Status: ");
        client.print(getStatusText(readings.temperatureStatus));
        client.println("</div>");

        client.print("<div class='sensor-timestamp'>Last reading: ");
        client.print(formatTimestamp(readings.temperatureTimestamp));
        client.println("</div>");

        client.println("</div>");

        // Humidity Card
        client.println("<div class='sensor-card'>");
        client.println("<div class='sensor-title'>💧 Humidity</div>");

        client.print("<div class='sensor-value ");
        client.print(getStatusColor(readings.humidityStatus));
        client.print("'>");
        if (!isnan(readings.humidity))
        {
            client.print(readings.humidity, 1);
            client.print("<span class='sensor-unit'>%</span>");
        }
        else
        {
            client.print("--");
        }
        client.println("</div>");

        client.print("<div class='sensor-timestamp'>Status: ");
        client.print(getStatusText(readings.humidityStatus));
        client.println("</div>");

        client.print("<div class='sensor-timestamp'>Last reading: ");
        client.print(formatTimestamp(readings.humidityTimestamp));
        client.println("</div>");

        client.println("</div>");
    }

    client.println("</div>"); // End sensor-grid
    client.println("</div>"); // End container
    client.println("</body></html>");
}

void NetworkConnections::sendSensorDataJSON(WiFiClient &client, const LatestReadings &readings)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();

    client.println("{");
    client.println("  \"device\": \"Garden Guardian\",");
    client.print("  \"timestamp\": ");
    client.print(getRTCTime());
    client.println(",");
    client.print("  \"wifi_ssid\": \"");
    client.print(WiFi.SSID());
    client.println("\",");
    client.print("  \"ip_address\": \"");
    client.print(WiFi.localIP());
    client.println("\",");
    client.print("  \"rssi\": ");
    client.print(WiFi.RSSI());
    client.println(",");
    client.println("  \"sensors\": [");

    if (readings.hasValidData)
    {
        // Temperature sensor data
        client.println("    {");
        client.println("      \"id\": \"Temperature\",");
        client.println("      \"type\": [\"Temperature\"],");
        client.print("      \"status\": ");
        client.print(readings.temperatureStatus);
        client.println(",");
        client.println("      \"units\": [\"°C\"],");
        client.print("      \"values\": [");
        if (!isnan(readings.temperature))
        {
            client.print(readings.temperature, 2);
        }
        else
        {
            client.print("null");
        }
        client.println("],");
        client.print("      \"timestamp\": ");
        client.print(readings.temperatureTimestamp);
        client.println("");
        client.println("    },");

        // Humidity sensor data
        client.println("    {");
        client.println("      \"id\": \"Humidity\",");
        client.println("      \"type\": [\"Humidity\"],");
        client.print("      \"status\": ");
        client.print(readings.humidityStatus);
        client.println(",");
        client.println("      \"units\": [\"%\"],");
        client.print("      \"values\": [");
        if (!isnan(readings.humidity))
        {
            client.print(readings.humidity, 2);
        }
        else
        {
            client.print("null");
        }
        client.println("],");
        client.print("      \"timestamp\": ");
        client.print(readings.humidityTimestamp);
        client.println("");
        client.println("    }");
    }

    client.println("  ]");
    client.println("}");
}

String NetworkConnections::formatTimestamp(unsigned long timestamp)
{
    if (timestamp == 0)
    {
        return "Unknown";
    }

    time_t time = (time_t)timestamp;
    struct tm *timeinfo = localtime(&time);

    if (timeinfo == nullptr)
    {
        return "Invalid time";
    }

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}

String NetworkConnections::getCurrentTimeString(const char *timezone)
{
    unsigned long currentTime = getRTCTime();
    if (currentTime == 0)
    {
        return "00:00:00";
    }

    // Set timezone and update time conversion
    setenv("TZ", timezone, 1);
    tzset();

    time_t time = (time_t)currentTime;
    struct tm *timeinfo = localtime(&time);

    if (timeinfo == nullptr)
    {
        return "00:00:00";
    }

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return String(buffer);
}

String NetworkConnections::getStatusText(int status)
{
    switch (status)
    {
    case 200:
        return "OK";
    case 400:
        return "Warning";
    case 500:
        return "Error";
    default:
        return "Unknown";
    }
}

String NetworkConnections::getStatusColor(int status)
{
    switch (status)
    {
    case 200:
        return "status-ok";
    case 400:
        return "status-warning";
    case 500:
        return "status-error";
    default:
        return "";
    }
}

//------------------------------------------------------------------------------
// Webview Structure Functions
//------------------------------------------------------------------------------
void NetworkConnections::sendHTTPHeader(WiFiClient &client, int statusCode)
{
    client.printf("HTTP/1.1 %d %s\r\n", statusCode, statusCode == 200 ? "OK" : "Bad Request");
    client.println("Content-Type: text/html; charset=UTF-8");
    client.println("Connection: close");
    client.println();
}

void NetworkConnections::sendHTMLHeader(WiFiClient &client, const char *title)
{
    client.println("<html><head>");
    client.println("<meta charset='UTF-8'>");
    client.printf("<title>%s</title>", title);
    client.println("<style>");
    // Common CSS styles
    client.println("body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; }");
    client.println(".container { width: 100%; max-width: 400px; margin: 50px auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.2); }");
    client.println("h1 { color: #fff; margin: 0; }");
    client.println("h2 { margin-bottom: 20px; }");
    client.println("p { color: #666; line-height: 1.6; }");
    client.println(".header { background:rgb(32, 141, 191); color: white; padding: 15px; text-align: center; border-radius: 10px 10px 0 0; }");
    client.println("button { background:rgb(35, 119, 180); color: white; border: none; padding: 10px 20px; border-radius: 5px; font-size: 16px; cursor: pointer; margin-top: 15px; }");
    client.println("</style>");
}

void NetworkConnections::sendPageHeader(WiFiClient &client)
{
    client.println("</head><body>");
    client.println("<div class='header'><h1>Garden Guardian</h1></div>");
    client.println("<div class='container'>");
}

void NetworkConnections::sendPageFooter(WiFiClient &client)
{
    client.println("</div></body></html>");
}

//------------------------------------------------------------------------------
// Advanced Configuration Functions
//------------------------------------------------------------------------------
void NetworkConnections::sendAdvancedConfigPage(WiFiClient &client, const DeviceSettings &settings)
{
    sendHTTPHeader(client);
    sendHTMLHeader(client, "Advanced Device Settings");

    // Additional CSS specific to advanced config page
    client.println("<style>");
    client.println("label { font-size: 14px; font-weight: bold; display: block; margin-top: 15px; text-align: left; }");
    client.println("input[type='number'], input[type='text'] { width: 100%; padding: 8px; margin-top: 5px; border: 1px solid #ccc; border-radius: 5px; box-sizing: border-box; }");
    client.println("input:focus { border-color: #208dbf; outline: none; }");
    client.println(".form-group { margin-bottom: 15px; }");
    client.println(".form-row { display: flex; gap: 10px; }");
    client.println(".form-row .form-group { flex: 1; }");
    client.println("button:disabled { background: #ccc; cursor: not-allowed; }");
    client.println(".help-text { font-size: 12px; color: #666; margin-top: 3px; }");
    client.println(".warning { background: #fff3cd; border: 1px solid #ffeaa7; border-radius: 5px; padding: 10px; margin: 15px 0; }");
    client.println("</style>");

    // JavaScript for input validation
    client.println("<script>");
    client.println("function validateForm() {");
    client.println("  var sleepDuration = document.getElementById('sleepDuration').value;");
    client.println("  var sensorInterval = document.getElementById('sensorInterval').value;");
    client.println("  var stabilizationTime = document.getElementById('stabilizationTime').value;");
    client.println("  var deviceID = document.getElementById('deviceID').value.trim();");
    client.println("  var idCode = document.getElementById('idCode').value.trim();");
    client.println("  var submitButton = document.getElementById('submitButton');");
    client.println("  ");
    client.println("  var isValid = sleepDuration > 0 && sensorInterval > 0 && stabilizationTime > 0 && deviceID !== '' && idCode !== '';");
    client.println("  submitButton.disabled = !isValid;");
    client.println("}");
    client.println("document.addEventListener('DOMContentLoaded', function() {");
    client.println("  var inputs = document.querySelectorAll('input');");
    client.println("  inputs.forEach(function(input) {");
    client.println("    input.addEventListener('input', validateForm);");
    client.println("  });");
    client.println("  validateForm();");
    client.println("});");
    client.println("</script>");

    sendPageHeader(client);

    // Main content
    client.println("<h2>⚙️ Advanced Device Settings</h2>");

    client.println("<div class='warning'>");
    client.println("<strong>⚠️ Warning:</strong> Changing these settings will restart the device. Make sure you understand the impact of each setting.");
    client.println("</div>");

    client.println("<form action='/advanced-config' method='POST'>");

    // Sleep Duration
    client.println("<div class='form-group'>");
    client.println("<label for='sleepDuration'>Sleep Duration (seconds):</label>");
    client.print("<input type='number' id='sleepDuration' name='sleepDuration' value='");
    client.print(settings.sleepDuration / 1000000ULL);
    client.println("' min='5' max='3600' required>");
    client.println("<div class='help-text'>Time device sleeps between wake cycles (5-3600 seconds)</div>");
    client.println("</div>");

    // Sensor Read Interval
    client.println("<div class='form-group'>");
    client.println("<label for='sensorInterval'>Sensor Read Interval (seconds):</label>");
    client.print("<input type='number' id='sensorInterval' name='sensorInterval' value='");
    client.print(settings.sensorReadInterval / 1000);
    client.println("' min='1' max='3600' required>");
    client.println("<div class='help-text'>Time between sensor readings (1-3600 seconds)</div>");
    client.println("</div>");

    // Sensor Stabilization Time
    client.println("<div class='form-group'>");
    client.println("<label for='stabilizationTime'>Sensor Stabilization Time (seconds):</label>");
    client.print("<input type='number' id='stabilizationTime' name='stabilizationTime' value='");
    client.print(settings.sensorStabilizationTime / 1000);
    client.println("' min='0' max='600' required>");
    client.println("<div class='help-text'>Time to wait before trusting sensor readings (0-600 seconds)</div>");
    client.println("</div>");

    // Device ID and ID Code in a row
    client.println("<div class='form-row'>");
    client.println("<div class='form-group'>");
    client.println("<label for='deviceID'>Device ID:</label>");
    client.print("<input type='text' id='deviceID' name='deviceID' value='");
    client.print(settings.deviceID);
    client.println("' maxlength='20' required>");
    client.println("<div class='help-text'>Unique device identifier</div>");
    client.println("</div>");

    client.println("<div class='form-group'>");
    client.println("<label for='idCode'>ID Code:</label>");
    client.print("<input type='text' id='idCode' name='idCode' value='");
    client.print(settings.idCode);
    client.println("' maxlength='16' required>");
    client.println("<div class='help-text'>Device access point suffix</div>");
    client.println("</div>");
    client.println("</div>");

    client.println("<button type='submit' id='submitButton'>Save Settings & Restart</button>");
    client.println("</form>");

    // Navigation back
    client.println("<div style='margin-top: 20px; text-align: center;'>");
    client.println("<a href='/' style='text-decoration: none; color: #208dbf;'>← Back to Dashboard</a>");
    client.println("</div>");

    sendPageFooter(client);
}

void NetworkConnections::processAdvancedConfig(WiFiClient &client, String request)
{
    SysLogs::logInfo("NETWORK", "Received Advanced Configuration Request:");
    SysLogs::println(request);

    // Extract the POST body
    int bodyIndex = request.indexOf("\r\n\r\n");
    if (bodyIndex == -1)
    {
        SysLogs::logInfo("NETWORK", "Error: Could not locate POST body.");
        return;
    }
    request = request.substring(bodyIndex + 4);

    SysLogs::logInfo("NETWORK", "Extracted POST Body:");
    SysLogs::println(request);

    DeviceSettings newSettings;

    // Parse sleep duration
    int sleepStart = request.indexOf("sleepDuration=");
    if (sleepStart != -1)
    {
        sleepStart += 14; // Move past "sleepDuration="
        int sleepEnd = request.indexOf("&", sleepStart);
        if (sleepEnd == -1)
            sleepEnd = request.length();
        String sleepStr = request.substring(sleepStart, sleepEnd);
        newSettings.sleepDuration = sleepStr.toInt() * 1000000ULL; // Convert to microseconds
    }

    // Parse sensor interval
    int intervalStart = request.indexOf("sensorInterval=");
    if (intervalStart != -1)
    {
        intervalStart += 15; // Move past "sensorInterval="
        int intervalEnd = request.indexOf("&", intervalStart);
        if (intervalEnd == -1)
            intervalEnd = request.length();
        String intervalStr = request.substring(intervalStart, intervalEnd);
        newSettings.sensorReadInterval = intervalStr.toInt() * 1000; // Convert to milliseconds
    }

    // Parse stabilization time
    int stabilizationStart = request.indexOf("stabilizationTime=");
    if (stabilizationStart != -1)
    {
        stabilizationStart += 18; // Move past "stabilizationTime="
        int stabilizationEnd = request.indexOf("&", stabilizationStart);
        if (stabilizationEnd == -1)
            stabilizationEnd = request.length();
        String stabilizationStr = request.substring(stabilizationStart, stabilizationEnd);
        newSettings.sensorStabilizationTime = stabilizationStr.toInt() * 1000; // Convert to milliseconds
    }

    // Parse device ID
    int deviceIDStart = request.indexOf("deviceID=");
    if (deviceIDStart != -1)
    {
        deviceIDStart += 9; // Move past "deviceID="
        int deviceIDEnd = request.indexOf("&", deviceIDStart);
        if (deviceIDEnd == -1)
            deviceIDEnd = request.length();
        newSettings.deviceID = urlDecode(request.substring(deviceIDStart, deviceIDEnd));
        newSettings.deviceID.replace("+", " ");
    }

    // Parse ID code
    int idCodeStart = request.indexOf("idCode=");
    if (idCodeStart != -1)
    {
        idCodeStart += 7; // Move past "idCode="
        String idCodeStr = request.substring(idCodeStart);
        newSettings.idCode = urlDecode(idCodeStr);
        newSettings.idCode.replace("+", " ");
    }

    SysLogs::logInfo("NETWORK", "Parsed Settings:");
    SysLogs::print("Sleep Duration: ");
    SysLogs::print(String(newSettings.sleepDuration / 1000000ULL));
    SysLogs::logInfo("NETWORK", " seconds");
    SysLogs::print("Sensor Interval: ");
    SysLogs::print(String(newSettings.sensorReadInterval / 1000));
    SysLogs::logInfo("NETWORK", " seconds");
    SysLogs::print("Stabilization Time: ");
    SysLogs::print(String(newSettings.sensorStabilizationTime / 1000));
    SysLogs::logInfo("NETWORK", " seconds");
    SysLogs::print("Device ID: ");
    SysLogs::println(newSettings.deviceID);
    SysLogs::print("ID Code: ");
    SysLogs::println(newSettings.idCode);

    // Validate settings
    bool isValid = (newSettings.sleepDuration >= 5000000ULL && newSettings.sleepDuration <= 3600000000ULL) &&
                   (newSettings.sensorReadInterval >= 1000 && newSettings.sensorReadInterval <= 3600000) &&
                   (newSettings.sensorStabilizationTime <= 600000) &&
                   (newSettings.deviceID.length() > 0 && newSettings.deviceID.length() <= 20) &&
                   (newSettings.idCode.length() > 0 && newSettings.idCode.length() <= 16);

    if (isValid)
    {
        saveDeviceSettings(newSettings);

        sendHTTPHeader(client);
        sendHTMLHeader(client, "Settings Saved");

        client.println("<style>");
        client.println(".success-icon { font-size: 48px; color: #28a745; margin: 20px 0; }");
        client.println("h2 { color: #28a745; }</style>");

        sendPageHeader(client);

        client.println("<div class='success-icon'>✅</div>");
        client.println("<h2>Settings Saved Successfully!</h2>");
        client.println("<p>Your device settings have been saved and the device will now restart.</p>");
        client.println("<p>The device will use the new settings after restart.</p>");

        sendPageFooter(client);

        delay(3000);
        ESP.restart();
    }
    else
    {
        sendHTTPHeader(client, 400);
        sendHTMLHeader(client, "Invalid Settings");

        client.println("<style>");
        client.println(".error-icon { font-size: 48px; color: #dc3545; margin: 20px 0; }");
        client.println("h2 { color: #dc3545; }</style>");

        client.println("<script>");
        client.println("function goBack() { window.history.back(); }");
        client.println("</script>");

        sendPageHeader(client);

        client.println("<div class='error-icon'>❌</div>");
        client.println("<h2>Error: Invalid Settings</h2>");
        client.println("<p>Please check that all values are within the valid ranges:</p>");
        client.println("<ul style='text-align: left; max-width: 300px; margin: 0 auto;'>");
        client.println("<li>Sleep Duration: 5-3600 seconds</li>");
        client.println("<li>Sensor Interval: 1-3600 seconds</li>");
        client.println("<li>Stabilization Time: 0-600 seconds</li>");
        client.println("<li>Device ID: 1-20 characters</li>");
        client.println("<li>ID Code: 1-16 characters</li>");
        client.println("</ul>");
        client.println("<button onclick='goBack()'>Try Again</button>");

        sendPageFooter(client);
    }
}

bool NetworkConnections::retryNTPSync()
{
    // Check if we're connected to WiFi
    if (WiFi.status() != WL_CONNECTED)
    {
        SysLogs::logInfo("NETWORK", "Cannot retry NTP sync - not connected to WiFi");
        return false;
    }

    // Check if RTC time is already reasonably current (within last hour)
    unsigned long currentRTC = getRTCTime();
    if (currentRTC > 0)
    {
        // If RTC is already current (e.g., sync was recent), don't retry immediately
        // This prevents excessive NTP requests
        SysLogs::logInfo("NETWORK", "RTC time appears current, skipping NTP retry");
        return true; // Consider this success since we have valid time
    }

    SysLogs::logInfo("NETWORK", "Attempting periodic NTP synchronization retry...");

    // Use a simpler, faster approach for retry attempts
    const char *quickServer = "pool.ntp.org";
    const unsigned long QUICK_TIMEOUT = 8000; // 8 seconds for retry

    configTime(0, 0, quickServer);

    unsigned long startTime = millis();
    struct tm timeinfo;

    SysLogs::print("Quick NTP sync");
    while ((millis() - startTime < QUICK_TIMEOUT))
    {
        if (getLocalTime(&timeinfo))
        {
            SysLogs::println();
            SysLogs::logInfo("NETWORK", "Periodic NTP sync successful!");
            return true;
        }
        SysLogs::print(".");
        delay(500);
        esp_task_wdt_reset();
    }

    SysLogs::println();
    SysLogs::logInfo("NETWORK", "Periodic NTP sync failed - will try again later");
    return false;
}

//------------------------------------------------------------------------------
// HTTP Publishing Functions
//------------------------------------------------------------------------------

bool NetworkConnections::testServerConnection(const String &device_id)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        SysLogs::logInfo("NETWORK", "[HTTP] Cannot test server connection - not connected to WiFi");
        return false;
    }

    HTTPClient http;
    serverData server;

    // Combine device_id with idCode for the full device identifier
    String queryString = "?deviceID=" + device_id;
    String url = "http://" + String(server.address) + ":" + String(server.port) + server.test + queryString;

    SysLogs::logInfo("HTTP", "Testing server connection to: " + url);
    SysLogs::logInfo("HTTP", "Device ID for test: " + device_id);

    http.begin(url);
    http.setTimeout(10000); // 10 second timeout

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
        SysLogs::logInfo("NETWORK", "[HTTP] Server ping successful: " + String(httpResponseCode) + "");
        String response = http.getString();
        SysLogs::logDebug("HTTP", "Server response: " + response);
        http.end();
        return httpResponseCode == 200;
    }
    else
    {
        SysLogs::logInfo("NETWORK", "[HTTP] Server ping failed with error: " + String(httpResponseCode) + "");
        http.end();
        return false;
    }
}

bool NetworkConnections::sendSensorDataHTTP(const sensorData &data, const String &deviceID)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        SysLogs::logInfo("NETWORK", "[HTTP] Cannot send data - not connected to WiFi");
        return false;
    }

    HTTPClient http;
    serverData server;

    String url = "http://" + String(server.address) + ":" + String(server.port) + server.apiPostRoute;

    SysLogs::logInfo("HTTP", "Sending sensor data to: " + url);

    // Create JSON payload using ArduinoJson v7 syntax
    JsonDocument doc;
    doc["deviceId"] = deviceID;
    doc["sensorId"] = data.sensorID;
    doc["sensorType"] = data.sensorType[0]; // Use first sensor type
    doc["status"] = data.status;
    doc["unit"] = data.unit[0]; // Use first unit
    doc["timestamp"] = data.timestamp;

    // Add sensor values array using v7 syntax
    JsonArray values = doc["values"].to<JsonArray>();
    for (float value : data.values)
    {
        values.add(value);
    }

    String jsonString;
    serializeJson(doc, jsonString);

    SysLogs::logDebug("HTTP", "JSON payload: " + jsonString);

    // Configure HTTP request
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32-GardenGuardian/1.0");
    http.setTimeout(15000); // 15 second timeout

    // Send POST request
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        SysLogs::logInfo("NETWORK", "[HTTP] Response code: " + String(httpResponseCode) + "");
        SysLogs::logDebug("HTTP", "Response: " + response);

        http.end();
        return httpResponseCode >= 200 && httpResponseCode < 300; // Accept 2xx responses
    }
    else
    {
        SysLogs::logInfo("NETWORK", "[HTTP] Request failed with error: " + String(httpResponseCode) + "");
        http.end();
        return false;
    }
}

bool NetworkConnections::publishSensorData(const SensorDataManager &dataManager, const String &deviceID)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        SysLogs::logInfo("NETWORK", "[HTTP] Cannot publish data - not connected to WiFi");
        return false;
    }

    SysLogs::logInfo("NETWORK", "[HTTP] Starting sensor data publication...");

    // Test server connection first
    if (!testServerConnection(deviceID))
    {
        SysLogs::logInfo("NETWORK", "[HTTP] Server connection test failed - aborting publication");
        return false;
    }
    // Get all sensor data
    std::vector<struct sensorData> allData = dataManager.getAllSensorData();

    if (allData.empty())
    {
        SysLogs::logInfo("NETWORK", "[HTTP] No sensor data to publish");
        return true; // Not an error, just nothing to send
    }

    SysLogs::logInfo("HTTP", "Publishing " + String(allData.size()) + " sensor data items...");

    int successCount = 0;
    int totalCount = allData.size();

    // Send each sensor data item separately
    for (const auto &data : allData)
    {
        SysLogs::logDebug("HTTP", "Sending data for sensor: " + data.sensorID);

        if (sendSensorDataHTTP(data, deviceID))
        {
            successCount++;
            SysLogs::logSuccess("HTTP", "Successfully sent data for sensor: " + data.sensorID);
        }
        else
        {
            SysLogs::logError("Failed to send data for sensor: " + data.sensorID);
        }

        // Small delay between requests to avoid overwhelming the server
        delay(100);

        // Feed watchdog to prevent reset during multiple HTTP requests
        esp_task_wdt_reset();
    }

    SysLogs::logInfo("NETWORK", "[HTTP] Publication complete: " + String(successCount) + "/" + String(totalCount) + " successful");

    // Return true if at least some data was sent successfully
    return successCount > 0;
}