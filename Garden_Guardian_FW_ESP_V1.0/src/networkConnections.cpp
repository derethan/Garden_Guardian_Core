#include "NetworkConnections.h"

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
int wifiStatus = WL_IDLE_STATUS; // WiFi radio status
int status = WL_IDLE_STATUS;     // WiFi connection status
bool apMode = false;             // Tracks if the device is in AP mode
String availableNetworks = "";   // Stores the Wi-Fi list for later use
char ssid[25] = "";
char password[25] = "";

WiFiServer server(80); // Create a server object
static WiFiUDP ntpUDP; // Used for NTP requests

//------------------------------------------------------------------------------
// Initialization Functions
//------------------------------------------------------------------------------
void NetworkConnections::setupWiFi(WiFiCredentials credentials, String idCode, bool apOn)
{
    // If ApOn is true, then the AP mode is always on, use Wifi Mode for both Station and Ap
    if (apOn)
    {
        Serial.println("Configuring dual mode (AP + Station)");
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
                Serial.println("Failed to connect to WiFi, but AP remains active");
            }
        }
    }
    else
    {
        Serial.println("Configuring station mode only");
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
#include <Preferences.h> // Required for NVS
#include <esp_task_wdt.h>
Preferences preferences;

WiFiCredentials NetworkConnections::loadWiFiCredentials()
{
    WiFiCredentials credentials;
    credentials.valid = false; // Default to invalid

    // Attempt to load credentials from NVS
    Serial.println("Loading Credentials from NVS storage...");
    preferences.begin("wifi", true); // Read-only mode
    credentials.ssid = preferences.getString("ssid", "");
    credentials.password = preferences.getString("password", "");
    preferences.end();

    if (credentials.ssid.length() > 0 && credentials.password.length() > 0)
    {
        credentials.valid = true;
        Serial.println("Wi-Fi credentials loaded successfully from NVS.");
        Serial.print("SSID: ");
        Serial.println(credentials.ssid);
    }
    else
    {
        Serial.println("No Wi-Fi credentials found in NVS. Starting AP mode.");
    }

    return credentials;
}

bool NetworkConnections::connectToNetwork(String ssid, String password)
{
    // Define timeout period (20 seconds)
    const unsigned long TIMEOUT = 20000; // 20 seconds in milliseconds
    unsigned long startAttempt = millis();

    Serial.print("Attempting to connect to SSID: ");
    Serial.print(ssid);

    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection or timeout
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < TIMEOUT)
    {
        Serial.print(".");
        delay(500); // Check every 500ms

        // Print progress every 3 seconds
        if ((millis() - startAttempt) % 3000 == 0)
        {
            Serial.printf("\nStill trying to connect... (%.1f seconds elapsed)\n",
                          (millis() - startAttempt) / 1000.0);
        }
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Connected to WiFi network: ");
        Serial.println(ssid);

        // Update the time
        getTime();

        return true; // Successfully connected
    }
    else
    {
        Serial.println("WiFi connection failed.");
        WiFi.disconnect(); // Clean up the failed connection attempt
        return false;      // Connection failed
    }
}

unsigned long NetworkConnections::getTime()
{
    const char *ntpServer = "pool.ntp.org";

    // Configure NTP time synchronization with UTC (no timezone offset)
    configTime(0, 0, ntpServer);

    // Get time from NTP server
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        Serial.println("Failed to obtain time from NTP");
        return 0;
    }

    // Update ESP32's RTC
    struct timezone tz = {0, 0};
    settimeofday(&tv, &tz);

    // Return Unix timestamp
    return (unsigned long)tv.tv_sec;
}

unsigned long NetworkConnections::getRTCTime()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        Serial.println("Failed to get time from RTC");
        return 0; // Return 0 if RTC read fails
    }

    // Check if time is unreasonable (before 2024)
    // Unix timestamp for Jan 1, 2024: 1704067200
    if (tv.tv_sec < 1704067200)
    {
        Serial.println("RTC time appears invalid (before 2024)");
        return 0;
    }

    return (unsigned long)tv.tv_sec;
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
    Serial.print("Creating access point named: ");
    Serial.println(apSSID);

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
        Serial.println("Creating access point failed");
        return;
    }

    apMode = true;

    dnsServer.start(53, "*", local_IP);
    server.begin();

    Serial.println("Access Point started successfully");
    Serial.print("AP SSID: ");
    Serial.println(apSSID);
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
}

void NetworkConnections::scanNetworks()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Scanning for WiFi networks...");
    esp_task_wdt_reset(); // Feed the watchdog in the loop
    
    int numNetworks = WiFi.scanNetworks();


    Serial.print("Number of networks found: ");
    Serial.println(numNetworks);

    if (numNetworks <= 0)
    {
        Serial.println("No networks found or scan failed.");
        availableNetworks = "<option value=''>No Networks Found</option>";
    }
    else
    {
        Serial.println("Networks found:");
        availableNetworks = ""; // Clear previous results

        for (int i = 0; i < numNetworks; i++)
        {
            esp_task_wdt_reset(); // Feed the watchdog in the loop

            availableNetworks += "<option value='";
            availableNetworks += WiFi.SSID(i);
            availableNetworks += "'>";
            availableNetworks += WiFi.SSID(i);
            availableNetworks += "</option>";

            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (RSSI: ");
            Serial.print(WiFi.RSSI(i));
            Serial.print(" dBm) ");
            Serial.print(" [");
            Serial.print(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured");
            Serial.println("]");
            delay(10);
        }
    }
}

void NetworkConnections::handleClientRequests()
{
    WiFiClient client = server.available();
    if (client)
    {
        Serial.println("New Client Connected!");
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

        Serial.println("Full HTTP Request:");
        Serial.println(request);

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
        Serial.println("Client Disconnected.");
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

    sendPageFooter(client);
}

void NetworkConnections::processWiFiConfig(WiFiClient &client, String request)
{
    Serial.println("Received Wi-Fi Configuration Request:");
    Serial.println(request); // Log the full request

    // **Step 1: Extract the POST body correctly**
    int bodyIndex = request.indexOf("\r\n\r\n"); // Find where headers end
    if (bodyIndex == -1)
    {
        Serial.println("Error: Could not locate POST body.");
        return;
    }
    request = request.substring(bodyIndex + 4); // The actual POST data

    Serial.println("Extracted POST Body:");
    Serial.println(request); // Should now show 'ssid=MySSID&password=MyPass'

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

    Serial.print("Extracted SSID: ");
    Serial.println(ssid);
    Serial.print("Extracted Password: ");
    Serial.println(password);

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
    Serial.println("Saving Wi-Fi credentials to NVS...");
    preferences.begin("wifi", false); // Read-write mode
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    Serial.println("Wi-Fi credentials successfully saved to NVS.");
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
    Serial.println("---------------Network Info-------------");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("----------------------------------------");
}
//------------------------------------------------------------------------------
// Webview Structure Functions
//------------------------------------------------------------------------------
void NetworkConnections::sendHTTPHeader(WiFiClient &client, int statusCode)
{
    client.printf("HTTP/1.1 %d %s\r\n", statusCode, statusCode == 200 ? "OK" : "Bad Request");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
}

void NetworkConnections::sendHTMLHeader(WiFiClient &client, const char *title)
{
    client.println("<html><head>");
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
    client.println("<div class='header'><h1>Garden Guardian Configuration Page</h1></div>");
    client.println("<div class='container'>");
}

void NetworkConnections::sendPageFooter(WiFiClient &client)
{
    client.println("</div></body></html>");
}