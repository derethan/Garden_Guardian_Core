/**
 * @file serialConfig.cpp
 * @brief Implementation of serial configuration interface for device management
 *
 * Provides interactive CLI-based configuration menu accessible via serial connection.
 */

#include "../../include/base/serialConfig.h"
#include "../../include/base/sysLogs.h"
#include "../../include/config.h"
#include <WiFi.h>

namespace SerialCLI
{
    // Static buffer for serial input accumulation
    static String serialInputBuffer = "";

    /**
     * @brief Check if user has entered the serial access password
     */
    bool checkForSerialAccess()
    {

        while (Serial.available() > 0)
        {

            char c = Serial.read();
            // Print Serial
            Serial.print(c);

            if (c == '\n' || c == '\r')
            {
                // Check if accumulated input matches password
                if (serialInputBuffer == SERIAL_ACCESS_PASSWORD)
                {
                    serialInputBuffer = ""; // Clear buffer
                    return true;
                }
                else if (serialInputBuffer == "debug on")
                {
                    DEBUG_MODE = true;
                    Serial.println("\nDebug mode enabled.");
                    serialInputBuffer = ""; // Clear buffer
                    return false;
                }
                else if (serialInputBuffer == "debug off")
                {
                    DEBUG_MODE = false;
                    Serial.println("\nDebug mode disabled.");
                    serialInputBuffer = ""; // Clear buffer
                    return false;
                }
                else
                {
                    serialInputBuffer = ""; // Clear buffer
                    return false;
                }
                serialInputBuffer = ""; // Clear buffer if no match
            }
            else
            {
                serialInputBuffer += c;
                // Prevent buffer overflow
                if (serialInputBuffer.length() > 50)
                {
                    serialInputBuffer = "";
                }
            }
        }
        return false;
    }

    /**
     * @brief Main entry point for serial configuration mode
     */
    void enterSerialMode(SystemState &state, NetworkConnections &network,
                         DHTSensor &dhtSensor, LatestReadings &latestReadings)
    {
        // Store previous DEBUG_MODE state (global variable declared in config.h)
        bool previousDebugMode = DEBUG_MODE;

        // Disable debug mode to prevent log spam
        DEBUG_MODE = false;

        // Save previous mode and track entry time
        state.serialModeStartTime = millis();

        // Clear serial buffer
        while (Serial.available() > 0)
        {
            Serial.read();
        }

        Serial.println();
        Serial.println();
        displaySectionHeader("SERIAL CONFIGURATION MODE");
        Serial.println("Debug logging has been disabled.");
        Serial.println("Type 'exit' at any time to return to normal operation.");
        Serial.println();

        // Main menu loop
        bool continueMode = true;
        while (continueMode)
        {
            // Check for timeout
            if (millis() - state.serialModeStartTime > SERIAL_TIMEOUT)
            {
                Serial.println();
                Serial.println("Session timeout. Returning to normal operation...");
                break;
            }

            displayMainMenu();
            continueMode = processMenuSelection(state, network, dhtSensor, latestReadings);
        }

        // Restore previous state
        DEBUG_MODE = previousDebugMode;

        state.currentMode = state.previousMode;

        // log the state
        SysLogs::logInfo("SYSTEM", "Exiting serial mode. Previous mode: " + String((int)state.previousMode) + ", Current mode: " + String((int)state.currentMode));

        Serial.println();
        displaySectionHeader("EXITING SERIAL MODE");
        Serial.println("Debug logging has been re-enabled.");
        Serial.println("Returning to normal operation...");
        Serial.println();
    }

    /**
     * @brief Display the main configuration menu
     */
    void displayMainMenu()
    {
        Serial.println();
        displaySeparator();
        Serial.println("          MAIN CONFIGURATION MENU");
        displaySeparator();
        Serial.println(" 1. Network Information");
        Serial.println(" 2. System Information");
        Serial.println(" 3. Configure State Settings");
        Serial.println(" 4. Configure Network Settings");
        Serial.println(" 5. Configure Device Settings");
        Serial.println(" 6. Run Diagnostics");
        Serial.println(" 0. Exit Serial Mode");
        displaySeparator();
        Serial.print("Select an option: ");
    }

    /**
     * @brief Read and process user menu selection
     */
    bool processMenuSelection(SystemState &state, NetworkConnections &network,
                              DHTSensor &dhtSensor, LatestReadings &latestReadings)
    {
        String input = readSerialInput(60000); // 60 second timeout
        input.trim();

        if (input.length() == 0)
        {
            Serial.println("\nNo input received. Try again.");
            return true;
        }

        // Check for exit command
        if (input.equalsIgnoreCase("exit") || input == "0")
        {
            return false;
        }

        Serial.println(); // New line after input

        switch (input.toInt())
        {
        case 1:
            displayNetworkInfo(network);
            waitForEnter();
            break;
        case 2:
            displaySystemInfo(state);
            waitForEnter();
            break;
        case 3:
            configureStateSettings(state, network);
            break;
        case 4:
            configureNetworkSettings(network);
            break;
        case 5:
            configureDeviceSettings(state, network);
            break;
        case 6:
            runDiagnostics(state, dhtSensor, latestReadings);
            waitForEnter();
            break;
        default:
            Serial.println("Invalid option. Please try again.");
            delay(1000);
            break;
        }

        return true;
    }

    /**
     * @brief Display network information
     */
    void displayNetworkInfo(NetworkConnections &network)
    {
        displaySectionHeader("NETWORK INFORMATION");

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("Status: Connected");
            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.print("Gateway: ");
            Serial.println(WiFi.gatewayIP());
            Serial.print("Subnet Mask: ");
            Serial.println(WiFi.subnetMask());
            Serial.print("DNS Server: ");
            Serial.println(WiFi.dnsIP());
            Serial.print("MAC Address: ");
            Serial.println(WiFi.macAddress());
            Serial.print("RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            Serial.print("Channel: ");
            Serial.println(WiFi.channel());
        }
        else if (network.isAPMode())
        {
            Serial.println("Status: Access Point Mode");
            Serial.print("AP SSID: ");
            Serial.println(WiFi.softAPSSID());
            Serial.print("AP IP: ");
            Serial.println(WiFi.softAPIP());
            Serial.print("Connected Clients: ");
            Serial.println(WiFi.softAPgetStationNum());
        }
        else
        {
            Serial.println("Status: Disconnected");
        }

        Serial.println();
    }

    /**
     * @brief Display system information
     */
    void displaySystemInfo(SystemState &state)
    {
        displaySectionHeader("SYSTEM INFORMATION");

        Serial.print("Device ID: ");
        Serial.println(state.deviceID);
        Serial.print("ID Code: ");
        Serial.println(state.idCode);
        Serial.print("Current Mode: ");
        switch (state.currentMode)
        {
        case SystemMode::INITIALIZING:
            Serial.println("INITIALIZING");
            break;
        case SystemMode::NORMAL_OPERATION:
            Serial.println("NORMAL_OPERATION");
            break;
        case SystemMode::CONFIG_MODE:
            Serial.println("CONFIG_MODE");
            break;
        case SystemMode::WAKE_UP:
            Serial.println("WAKE_UP");
            break;
        case SystemMode::SERIAL_MODE:
            Serial.println("SERIAL_MODE");
            break;
        case SystemMode::ERROR:
            Serial.println("ERROR");
            break;
        default:
            Serial.println("UNKNOWN");
            break;
        }

        Serial.print("Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");

        Serial.print("Free Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");

        Serial.print("Chip Model: ");
        Serial.println(ESP.getChipModel());

        Serial.print("Chip Revision: ");
        Serial.println(ESP.getChipRevision());

        Serial.print("CPU Frequency: ");
        Serial.print(ESP.getCpuFreqMHz());
        Serial.println(" MHz");

        Serial.print("Flash Size: ");
        Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
        Serial.println(" MB");

        if (state.sensorError)
        {
            Serial.println();
            Serial.println("*** SENSOR ERROR DETECTED ***");
            Serial.print("Last Error: ");
            Serial.println(state.lastErrorMessage);
        }

        Serial.println();
    }

    /**
     * @brief Display and modify state configuration settings
     */
    void configureStateSettings(SystemState &state, NetworkConnections &network)
    {
        displaySectionHeader("STATE CONFIGURATION");

        Serial.println("Current Settings:");
        Serial.print("1. Sensor Read Interval: ");
        Serial.print(state.sensorRead_interval / 1000);
        Serial.println(" seconds");

        Serial.print("2. HTTP Publish Interval: ");
        Serial.print(state.httpPublishInterval / 1000);
        Serial.println(" seconds");

        Serial.print("3. HTTP Publish Enabled: ");
        Serial.println(state.httpPublishEnabled ? "Yes" : "No");

        Serial.print("4. Sensor Stabilization Time: ");
        Serial.print(state.SENSOR_STABILIZATION_TIME / 1000);
        Serial.println(" seconds");

        Serial.print("5. Sleep Duration: ");
        Serial.print(state.SLEEP_DURATION / 1000000);
        Serial.println(" seconds");

        Serial.println("0. Back to Main Menu");
        Serial.println();
        Serial.print("Select setting to modify (or 0 to return): ");

        String input = readSerialInput();
        input.trim();

        if (input == "0" || input.length() == 0)
        {
            return;
        }

        Serial.println();
        bool settingChanged = false;

        switch (input.toInt())
        {
        case 1:
            Serial.print("Enter new Sensor Read Interval (seconds): ");
            input = readSerialInput();
            if (input.length() > 0)
            {
                unsigned long newValue = input.toInt() * 1000;
                if (newValue >= 5000)
                {
                    state.sensorRead_interval = newValue;
                    settingChanged = true;
                    Serial.println("Updated successfully!");
                }
                else
                {
                    Serial.println("Value too small (minimum 5 seconds)");
                }
            }
            break;

        case 2:
            Serial.print("Enter new HTTP Publish Interval (seconds): ");
            input = readSerialInput();
            if (input.length() > 0)
            {
                unsigned long newValue = input.toInt() * 1000;
                if (newValue >= 10000)
                {
                    state.httpPublishInterval = newValue;
                    settingChanged = true;
                    Serial.println("Updated successfully!");
                }
                else
                {
                    Serial.println("Value too small (minimum 10 seconds)");
                }
            }
            break;

        case 3:
            Serial.print("Enable HTTP Publishing? (y/n): ");
            input = readSerialInput();
            input.toLowerCase();
            if (input == "y" || input == "yes")
            {
                state.httpPublishEnabled = true;
                settingChanged = true;
                Serial.println("HTTP Publishing enabled");
            }
            else if (input == "n" || input == "no")
            {
                state.httpPublishEnabled = false;
                settingChanged = true;
                Serial.println("HTTP Publishing disabled");
            }
            break;

        case 4:
            Serial.print("Enter new Stabilization Time (seconds): ");
            input = readSerialInput();
            if (input.length() > 0)
            {
                unsigned long newValue = input.toInt() * 1000;
                state.SENSOR_STABILIZATION_TIME = newValue;
                settingChanged = true;
                Serial.println("Updated successfully!");
            }
            break;

        case 5:
            Serial.print("Enter new Sleep Duration (seconds): ");
            input = readSerialInput();
            if (input.length() > 0)
            {
                long seconds = input.toInt();
                if (seconds >= 1)
                {
                    state.SLEEP_DURATION = (uint64_t)seconds * 1000000ULL;
                    settingChanged = true;
                    Serial.println("Updated successfully!");
                }
                else
                {
                    Serial.println("Value too small (minimum 1 second)");
                }
            }
            break;

        default:
            Serial.println("Invalid option");
            break;
        }

        // Save settings to NVS if any changes were made
        if (settingChanged)
        {
            saveDeviceSettingsToNVS(state, network);
        }

        Serial.println();
        waitForEnter();
    }

    /**
     * @brief Configure network settings
     */
    void configureNetworkSettings(NetworkConnections &network)
    {
        displaySectionHeader("NETWORK CONFIGURATION");

        Serial.println("Network configuration options:");
        Serial.println("1. Scan for WiFi Networks");
        Serial.println("2. View Stored WiFi Credentials");
        Serial.println("3. View Network Configuration (IP, Gateway, DNS)");
        Serial.println("4. Configure Network Settings (IP, Gateway, DNS)");
        Serial.println("0. Back to Main Menu");
        Serial.println();
        Serial.print("Select option: ");

        String input = readSerialInput();
        input.trim();

        if (input == "0" || input.length() == 0)
        {
            return;
        }

        Serial.println();

        switch (input.toInt())
        {
        case 1:
            Serial.println("Scanning for networks...");
            {
                int n = WiFi.scanNetworks();
                Serial.print("Found ");
                Serial.print(n);
                Serial.println(" networks:");
                for (int i = 0; i < n; i++)
                {
                    Serial.print(i + 1);
                    Serial.print(". ");
                    Serial.print(WiFi.SSID(i));
                    Serial.print(" (");
                    Serial.print(WiFi.RSSI(i));
                    Serial.print(" dBm) ");
                    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "[Open]" : "[Secured]");
                }
            }
            break;

        case 2:
        {
            WiFiCredentials creds = network.loadWiFiCredentials();
            if (creds.valid)
            {
                Serial.print("SSID: ");
                Serial.println(creds.ssid);
                Serial.println("Password: ********");
            }
            else
            {
                Serial.println("No stored credentials found");
            }
            break;
        }

        case 3:
        {
            // View network configuration
            IPAddress ip, gateway, subnet, dns1, dns2;
            bool hasConfig = network.loadNetworkConfig(ip, gateway, subnet, dns1, dns2);

            Serial.println("Stored Network Configuration:");
            if (hasConfig)
            {
                Serial.print("IP Address: ");
                Serial.println(ip.toString());
                Serial.print("Gateway: ");
                Serial.println(gateway.toString());
                Serial.print("Subnet Mask: ");
                Serial.println(subnet.toString());
                Serial.print("DNS 1: ");
                Serial.println(dns1.toString());
                Serial.print("DNS 2: ");
                Serial.println(dns2.toString());
            }
            else
            {
                Serial.println("No network configuration stored (using DHCP)");
            }

            Serial.println();
            Serial.println("Current Active Network Configuration:");
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.print("IP Address: ");
                Serial.println(WiFi.localIP());
                Serial.print("Gateway: ");
                Serial.println(WiFi.gatewayIP());
                Serial.print("Subnet Mask: ");
                Serial.println(WiFi.subnetMask());
                Serial.print("DNS 1: ");
                Serial.println(WiFi.dnsIP(0));
                Serial.print("DNS 2: ");
                Serial.println(WiFi.dnsIP(1));
            }
            else
            {
                Serial.println("Not connected to WiFi");
            }
            break;
        }

        case 4:
        {
            // Configure network settings
            Serial.println("Configure Network Settings");
            Serial.println();
            Serial.println("Current configuration:");

            IPAddress ip, gateway, subnet, dns1, dns2;
            bool hasConfig = network.loadNetworkConfig(ip, gateway, subnet, dns1, dns2);

            if (!hasConfig)
            {
                // Use current WiFi settings as defaults
                ip = WiFi.localIP();
                gateway = WiFi.gatewayIP();
                subnet = WiFi.subnetMask();
                dns1 = WiFi.dnsIP(0);
                dns2 = WiFi.dnsIP(1);
            }

            Serial.print("IP Address: ");
            Serial.println(ip.toString());
            Serial.print("Gateway: ");
            Serial.println(gateway.toString());
            Serial.print("Subnet: ");
            Serial.println(subnet.toString());
            Serial.print("DNS 1: ");
            Serial.println(dns1.toString());
            Serial.print("DNS 2: ");
            Serial.println(dns2.toString());
            Serial.println();

            Serial.println("Enter new IP address (or press Enter to keep current): ");
            String ipStr = readSerialInput();
            if (ipStr.length() > 0)
            {
                ip.fromString(ipStr);
            }

            Serial.println("Enter new Gateway (or press Enter to keep current): ");
            String gwStr = readSerialInput();
            if (gwStr.length() > 0)
            {
                gateway.fromString(gwStr);
            }

            Serial.println("Enter new Subnet Mask (or press Enter to keep current): ");
            String subnetStr = readSerialInput();
            if (subnetStr.length() > 0)
            {
                subnet.fromString(subnetStr);
            }

            Serial.println("Enter new DNS 1 (or press Enter to keep current): ");
            String dns1Str = readSerialInput();
            if (dns1Str.length() > 0)
            {
                dns1.fromString(dns1Str);
            }

            Serial.println("Enter new DNS 2 (or press Enter to keep current): ");
            String dns2Str = readSerialInput();
            if (dns2Str.length() > 0)
            {
                dns2.fromString(dns2Str);
            }

            Serial.println();
            Serial.println("New configuration:");
            Serial.print("IP Address: ");
            Serial.println(ip.toString());
            Serial.print("Gateway: ");
            Serial.println(gateway.toString());
            Serial.print("Subnet: ");
            Serial.println(subnet.toString());
            Serial.print("DNS 1: ");
            Serial.println(dns1.toString());
            Serial.print("DNS 2: ");
            Serial.println(dns2.toString());
            Serial.println();

            Serial.print("Save this configuration? (y/n): ");
            String confirm = readSerialInput();
            confirm.toLowerCase();

            if (confirm == "y" || confirm == "yes")
            {
                network.saveNetworkConfig(ip, gateway, subnet, dns1, dns2);
                Serial.println("Network configuration saved to NVS!");
                Serial.println("Note: Changes will take effect after next WiFi connection.");
            }
            else
            {
                Serial.println("Configuration not saved.");
            }
            break;
        }

        default:
            Serial.println("Invalid option");
            break;
        }

        Serial.println();
        waitForEnter();
    }

    /**
     * @brief Display and modify device settings
     */
    void configureDeviceSettings(SystemState &state, NetworkConnections &network)
    {
        displaySectionHeader("DEVICE CONFIGURATION");

        Serial.println("Current Device Settings:");
        Serial.print("1. Device ID: ");
        Serial.println(state.deviceID);
        Serial.print("2. ID Code: ");
        Serial.println(state.idCode);
        Serial.println();
        Serial.println("0. Back to Main Menu");
        Serial.println();
        Serial.print("Select setting to modify (or 0 to return): ");

        String input = readSerialInput();
        input.trim();

        if (input == "0" || input.length() == 0)
        {
            return;
        }

        Serial.println();
        bool settingChanged = false;

        switch (input.toInt())
        {
        case 1:
            Serial.print("Enter new Device ID: ");
            input = readSerialInput();
            if (input.length() > 0)
            {
                state.deviceID = input;
                settingChanged = true;
                Serial.println("Device ID updated successfully!");
            }
            break;

        case 2:
            Serial.print("Enter new ID Code: ");
            input = readSerialInput();
            if (input.length() > 0)
            {
                state.idCode = input;
                settingChanged = true;
                Serial.println("ID Code updated successfully!");
            }
            break;

        default:
            Serial.println("Invalid option");
            break;
        }

        // Save settings to NVS if any changes were made
        if (settingChanged)
        {
            saveDeviceSettingsToNVS(state, network);
        }

        Serial.println();
        waitForEnter();
    }

    /**
     * @brief Run diagnostics and testing tools
     */
    void runDiagnostics(SystemState &state, DHTSensor &dhtSensor,
                        LatestReadings &latestReadings)
    {
        displaySectionHeader("DIAGNOSTICS & TESTING");

        Serial.println("Available Tests:");
        Serial.println("1. Read DHT Sensor");
        Serial.println("2. Test Memory");
        Serial.println("3. Network Connectivity Test");
        Serial.println("0. Back to Main Menu");
        Serial.println();
        Serial.print("Select test: ");

        String input = readSerialInput();
        input.trim();

        if (input == "0" || input.length() == 0)
        {
            return;
        }

        Serial.println();

        switch (input.toInt())
        {
        case 1:
        {
            Serial.println("Reading DHT sensor...");
            float temp = dhtSensor.readTemperature();
            float hum = dhtSensor.readHumidity();

            if (isnan(temp) || isnan(hum))
            {
                Serial.println("ERROR: Failed to read from DHT sensor!");
            }
            else
            {
                Serial.print("Temperature: ");
                Serial.print(temp);
                Serial.println(" °C");
                Serial.print("Humidity: ");
                Serial.print(hum);
                Serial.println(" %");

                if (latestReadings.hasValidData)
                {
                    Serial.println();
                    Serial.println("Latest Readings in System:");
                    Serial.print("  Temperature: ");
                    Serial.print(latestReadings.temperature);
                    Serial.println(" °C");
                    Serial.print("  Humidity: ");
                    Serial.print(latestReadings.humidity);
                    Serial.println(" %");
                }
            }
        }
        break;

        case 2:
        {
            Serial.println("Memory Test:");
            Serial.print("Free Heap: ");
            Serial.print(ESP.getFreeHeap());
            Serial.println(" bytes");
            Serial.print("Heap Size: ");
            Serial.print(ESP.getHeapSize());
            Serial.println(" bytes");
            Serial.print("Min Free Heap: ");
            Serial.print(ESP.getMinFreeHeap());
            Serial.println(" bytes");
            Serial.print("Max Alloc Heap: ");
            Serial.print(ESP.getMaxAllocHeap());
            Serial.println(" bytes");
        }
        break;

        case 3:
        {
            Serial.println("Network Connectivity Test:");
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("WiFi: Connected");
                Serial.print("Ping Gateway: ");
                // Note: Ping functionality would require additional implementation
                Serial.println("Not implemented");
            }
            else
            {
                Serial.println("WiFi: Not Connected");
            }
        }
        break;

        default:
            Serial.println("Invalid option");
            break;
        }

        Serial.println();
    }

    /**
     * @brief Read user input from serial with timeout
     */
    String readSerialInput(unsigned long timeoutMs)
    {
        String input = "";
        unsigned long startTime = millis();

        while (millis() - startTime < timeoutMs)
        {
            if (Serial.available() > 0)
            {
                char c = Serial.read();
                if (c == '\n' || c == '\r')
                {
                    if (input.length() > 0)
                    {
                        return input;
                    }
                }
                else
                {
                    input += c;
                    Serial.print(c); // Echo character
                }
            }
            delay(10);
        }

        return input;
    }

    /**
     * @brief Display a formatted section header
     */
    void displaySectionHeader(const String &title)
    {
        displaySeparator();
        Serial.print("  ");
        Serial.println(title);
        displaySeparator();
    }

    /**
     * @brief Display a separator line
     */
    void displaySeparator()
    {
        Serial.println("================================================");
    }

    /**
     * @brief Wait for user to press Enter to continue
     */
    void waitForEnter()
    {
        Serial.println();
        Serial.print("Press Enter to continue...");
        readSerialInput();
        Serial.println();
    }

    /**
     * @brief Helper function to save device settings to NVS
     */
    void saveDeviceSettingsToNVS(SystemState &state, NetworkConnections &network)
    {
        Serial.println("Saving settings to NVS...");
        // Load existing settings first to preserve values we're not modifying
        DeviceSettings settings = network.loadDeviceSettings();
        // Update with current state values
        settings.sleepDuration = state.SLEEP_DURATION;
        settings.sensorReadInterval = state.sensorRead_interval;
        settings.sensorStabilizationTime = state.SENSOR_STABILIZATION_TIME;
        settings.deviceID = state.deviceID;
        settings.idCode = state.idCode;
        settings.httpPublishEnabled = state.httpPublishEnabled;
        settings.httpPublishInterval = state.httpPublishInterval;
        settings.valid = true;
        // NTP settings are preserved from loaded settings

        network.saveDeviceSettings(settings);
        Serial.println("Settings saved to NVS successfully!");
    }

} // namespace SerialCLI
