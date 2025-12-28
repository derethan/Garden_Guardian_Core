/**
 * @file serialConfig.h
 * @brief Serial configuration interface for device management via CLI
 * 
 * Provides an interactive command-line interface for device configuration,
 * diagnostics, and system management through serial connection.
 * Activated by entering the configured password (default: "adminConfig").
 */

#ifndef BASE_SERIALCONFIG_H
#define BASE_SERIALCONFIG_H

#include <Arduino.h>
#include "../state.h"
#include "../networkConnections.h"
#include "../dhtSensor.h"
#include "../latestReadings.h"

/**
 * @namespace SerialCLI
 * @brief Serial configuration menu system for device management
 */
namespace SerialCLI
{
    /**
     * @brief Check if user has entered the serial access password
     * @return true if password was entered, false otherwise
     * 
     * Monitors serial input for the configured access password.
     * Non-blocking check that doesn't wait for input.
     */
    bool checkForSerialAccess();

    /**
     * @brief Main entry point for serial configuration mode
     * @param state Reference to system state object
     * @param network Reference to network connections object
     * @param dhtSensor Reference to DHT sensor object
     * @param latestReadings Reference to latest sensor readings
     * 
     * Disables debug logging and presents interactive menu system.
     * Handles all user interaction until exit command is given.
     */
    void enterSerialMode(SystemState &state, NetworkConnections &network, 
                        DHTSensor &dhtSensor, LatestReadings &latestReadings);

    /**
     * @brief Display the main configuration menu
     * 
     * Shows all available menu options to the user.
     */
    void displayMainMenu();

    /**
     * @brief Read and process user menu selection
     * @param state Reference to system state object
     * @param network Reference to network connections object
     * @param dhtSensor Reference to DHT sensor object
     * @param latestReadings Reference to latest sensor readings
     * @return true to continue in serial mode, false to exit
     * 
     * Handles user input and dispatches to appropriate menu handler.
     */
    bool processMenuSelection(SystemState &state, NetworkConnections &network,
                             DHTSensor &dhtSensor, LatestReadings &latestReadings);

    /**
     * @brief Display network information
     * @param network Reference to network connections object
     */
    void displayNetworkInfo(NetworkConnections &network);

    /**
     * @brief Display system information
     * @param state Reference to system state object
     */
    void displaySystemInfo(SystemState &state);

    /**
     * @brief Display and modify state configuration settings
     * @param state Reference to system state object
     * @param network Reference to network connections object
     */
    void configureStateSettings(SystemState &state, NetworkConnections &network);

    /**
     * @brief Configure network settings
     * @param network Reference to network connections object
     */
    void configureNetworkSettings(NetworkConnections &network);

    /**
     * @brief Display and modify device settings
     * @param state Reference to system state object
     * @param network Reference to network connections object
     */
    void configureDeviceSettings(SystemState &state, NetworkConnections &network);

    /**
     * @brief Run diagnostics and testing tools
     * @param state Reference to system state object
     * @param dhtSensor Reference to DHT sensor object
     * @param latestReadings Reference to latest sensor readings
     */
    void runDiagnostics(SystemState &state, DHTSensor &dhtSensor, 
                       LatestReadings &latestReadings);

    /**
     * @brief Read user input from serial with timeout
     * @param timeoutMs Timeout in milliseconds
     * @return User input string, empty if timeout
     * 
     * Waits for user input with specified timeout.
     */
    String readSerialInput(unsigned long timeoutMs = 30000);

    /**
     * @brief Display a formatted section header
     * @param title Section title to display
     */
    void displaySectionHeader(const String &title);

    /**
     * @brief Display a separator line
     */
    void displaySeparator();

    /**
     * @brief Wait for user to press Enter to continue
     */
    void waitForEnter();

    /**
     * @brief Helper function to save device settings to NVS
     * @param state Reference to system state object
     * @param network Reference to network connections object
     * 
     * Loads existing settings, updates with current state values, and saves to NVS.
     * Preserves settings that are not part of the system state (like NTP configuration).
     */
    void saveDeviceSettingsToNVS(SystemState &state, NetworkConnections &network);
} // namespace SerialCLI

#endif // BASE_SERIALCONFIG_H
