#ifndef SystemState_H
#define SystemState_H

#include <Arduino.h>
#include "config.h"

/*****************************************
 * Global State Variables
 *****************************************/
// Enum for system modes
enum class SystemMode
{
    INITIALIZING,
    NORMAL_OPERATION,
    ERROR,
    CONFIG_MODE
};

struct SystemState
{

    // Global Device Information
    String deviceID = DEVICE_ID; // Will be overridden by config
    String idCode = IDCODE;      // Will be overridden by config
    SystemMode currentMode = SystemMode::INITIALIZING;

    // Sleep settings
    uint64_t SLEEP_DURATION = 15ULL * 1000000ULL; // 15 seconds sleep
    const unsigned long MIN_WAKE_DURATION = 5000; // 5 seconds minimum wake time
    unsigned long wakeStartTime = 0;              // Track wake period start

    // Interval Timings
    unsigned long sensorRead_interval = 30000;       // 30 seconds between readings
    unsigned long SENSOR_STABILIZATION_TIME = 15000; // 15 seconds stabilization
    unsigned long httpPublishInterval = 30000;       // 30 seconds default

    // State Tracking for Last Operations
    unsigned long deviceStartTime = 0;
    unsigned long lastSensorRead = 0;
    unsigned long lastWiFiRetry = 0;
    unsigned long lastHTTPPublishTime = 0; // New timer for HTTP publishing
    uint32_t lastTimeSyncEpoch = 0;
    unsigned long currentTime = 0;
    uint32_t lastErrorTime = 0;
    unsigned long lastReadingTime = 0;

    // Error Handling
    String lastErrorMessage = "";

    // HTTP Publishing settings
    bool httpPublishEnabled = true;

    // Flags and Control Variables
    bool deviceStabilized = false;
    bool apAlwaysOn = false;
    bool sensorError = false;
    bool wasConnected = false;

    // Local Device Settings:
    unsigned long relayReadInterval = 60;
    unsigned long lastRelayRead = 0;

    unsigned long tdsControllerInterval = 5; // Check TDS controller every 5 seconds
    unsigned long lastTDSControllerRead = 0; // Add this new variable

    unsigned long relayScheduleOnHour = 0;
    unsigned long RelayScheduleOffHour = 18;
    // unsigned long relayTimerOnInterval = 0;
    // unsigned long relayTimerOffInterval = 0;

    float DWC_Res_Temp = 0;
    float Target_DWC_Res_Temp = 18;
    float NFT_Res_Temp = 0;
    float Target_NFT_Res_Temp = 18;
    float Current_Air_Temp = 0;
    float Target_Air_Temp = 25;

    float tdsValue = 0;
    float targetTDS = 500;     // Target TDS value - will be loaded from NVS
    float tdsHysteresis = 100; // Hysteresis for TDS control
};

// Global state variable declaration
extern SystemState state;

#endif // SystemState_H