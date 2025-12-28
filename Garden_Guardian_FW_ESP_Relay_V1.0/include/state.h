#pragma once
#include "Config.h"
#include <Arduino.h>

enum class SystemMode
{
  INITIALIZING,
  NORMAL_OPERATION,
  ERROR,
  CONFIG_MODE,
  WAKE_UP,
  SERIAL_MODE,
};

struct SystemState
{
  // Global Device Information
  SystemMode currentMode = SystemMode::INITIALIZING;
  String deviceID = DEVICE_ID; // Will be overridden by config (configurable)
  String idCode = IDCODE;      // Unique device identifier (configurable)

  // Sleep settings - now configurable
  uint64_t SLEEP_DURATION = 15ULL * 1000000ULL; // 15 seconds sleep (configurable)
  const unsigned long MIN_WAKE_DURATION = 5000; // 5 seconds minimum wake time
  unsigned long wakeStartTime = 0;              // Track wake period start
  bool sleepEnabled = false;

  // Sensor Interval Timings
  unsigned long sensorRead_interval = 60000;       // 60 seconds between readings (configurable)
  unsigned long SENSOR_STABILIZATION_TIME = 15000; // 15 seconds stabilization (configurable)
  bool deviceStabilized = false;

  // State Tracking for Last Operations
  unsigned long lastSensorRead = 0;
  unsigned long lastPublishTime = 0;
  unsigned long lastHTTPPublishTime = 0; // New timer for HTTP publishing
  uint32_t lastTimeSyncEpoch = 0;

  unsigned long currentTime = 0;
  unsigned long lastWiFiRetry = 0;

  bool wasConnected = false;
  unsigned long deviceStartTime = 0;
  unsigned long lastReadingTime = 0;

  // HTTP Publishing settings (configurable)
  bool httpPublishEnabled = true;
  unsigned long httpPublishInterval = 60000; // 60 seconds default

  bool sensorError = false;
  uint32_t lastErrorTime = 0;
  String lastErrorMessage;

  bool apAlwaysOn = false;

  unsigned long relayReadInterval = 60000; // 60 seconds
  unsigned long lastRelayRead = 0;
  unsigned long relayScheduleOnHour = 0;
  unsigned long RelayScheduleOffHour = 18;

  float DWC_Res_Temp = 0;
  float Target_DWC_Res_Temp = 18;
  float NFT_Res_Temp = 0;
  float Target_NFT_Res_Temp = 18;
  float Current_Air_Temp = 0;
  float Target_Air_Temp = 25;

  // Serial Mode tracking
  SystemMode previousMode = SystemMode::INITIALIZING; // Track mode before entering SERIAL_MODE
  unsigned long serialModeStartTime = 0;              // Track when SERIAL_MODE was entered
};

extern SystemState state;