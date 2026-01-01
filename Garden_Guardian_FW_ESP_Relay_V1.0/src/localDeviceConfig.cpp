/**
 * @file tempHumDeviceConfig.cpp
 * @brief Temperature/Humidity device specific configuration implementation
 * 
 * Implements the device-specific settings applier for temperature and humidity
 * monitoring devices, applying configuration values to the system state.
 */

#include "../include/localDeviceConfig.h"
#include "../include/base/sysLogs.h"
#include "state.h"

extern struct SystemState state;

/**
 * @brief Apply loaded settings to the device-specific state
 * @param settings The device settings structure containing configuration values
 */
void LocalDeviceSettingsApplier::applySettings(const DeviceSettings &settings)
{
    // Apply loaded settings to the device-specific state
    state.SLEEP_DURATION = settings.sleepDuration;
    state.sensorRead_interval = settings.sensorReadInterval;
    state.SENSOR_STABILIZATION_TIME = settings.sensorStabilizationTime;
    state.deviceID = settings.deviceID;
    state.idCode = settings.idCode;
    state.httpPublishEnabled = settings.httpPublishEnabled;
    state.httpPublishInterval = settings.httpPublishInterval;
}

/**
 * @brief Display the applied device settings to the log
 * @param settings The device settings structure to display
 */
void LocalDeviceSettingsApplier::displaySettings(const DeviceSettings &settings)
{
    SysLogs::logInfo("SYSTEM", "Device settings applied:");
    SysLogs::logInfo("SYSTEM", "  Sleep Duration: " + String(settings.sleepDuration / 1000000ULL) + " seconds");
    SysLogs::logInfo("SYSTEM", "  Sensor Read Interval: " + String(settings.sensorReadInterval / 1000) + " seconds");
    SysLogs::logInfo("SYSTEM", "  Stabilization Time: " + String(settings.sensorStabilizationTime / 1000) + " seconds");
    SysLogs::logInfo("SYSTEM", "  Device ID: " + settings.deviceID);
    SysLogs::logInfo("SYSTEM", "  ID Code: " + settings.idCode);
    SysLogs::logInfo("SYSTEM", "  HTTP Publishing: " + String(settings.httpPublishEnabled ? "Enabled" : "Disabled"));
    if (settings.httpPublishEnabled)
    {
        SysLogs::logInfo("SYSTEM", "  HTTP Publish Interval: " + String(settings.httpPublishInterval / 1000) + " seconds");
    }
}
