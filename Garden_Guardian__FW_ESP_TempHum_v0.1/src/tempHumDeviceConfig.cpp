#include "../include/tempHumDeviceConfig.h"

// Need to include main.cpp to access SystemState definition
extern struct SystemState state;

/*****************************************
 * Temperature/Humidity Device Specific Implementation
 *****************************************/

void TempHumDeviceSettingsApplier::applySettings(const DeviceSettings& settings) 
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

void TempHumDeviceSettingsApplier::displaySettings(const DeviceSettings& settings) 
{
    Serial.println("[SYSTEM] Device settings applied:");
    Serial.print("  Sleep Duration: ");
    Serial.print(settings.sleepDuration / 1000000ULL);
    Serial.println(" seconds");
    Serial.print("  Sensor Read Interval: ");
    Serial.print(settings.sensorReadInterval / 1000);
    Serial.println(" seconds");
    Serial.print("  Stabilization Time: ");
    Serial.print(settings.sensorStabilizationTime / 1000);
    Serial.println(" seconds");
    Serial.print("  Device ID: ");
    Serial.println(settings.deviceID);
    Serial.print("  ID Code: ");
    Serial.println(settings.idCode);
    Serial.print("  HTTP Publishing: ");
    Serial.println(settings.httpPublishEnabled ? "Enabled" : "Disabled");
    if (settings.httpPublishEnabled) 
    {
        Serial.print("  HTTP Publish Interval: ");
        Serial.print(settings.httpPublishInterval / 1000);
        Serial.println(" seconds");
    }
}
