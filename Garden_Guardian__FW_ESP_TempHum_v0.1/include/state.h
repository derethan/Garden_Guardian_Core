struct SystemState
{
  SystemMode currentMode = SystemMode::INITIALIZING;

  // Sleep settings - now configurable
  uint64_t SLEEP_DURATION = 15ULL * 1000000ULL; // 15 seconds sleep (configurable)
  const unsigned long MIN_WAKE_DURATION = 5000; // 5 seconds minimum wake time
  unsigned long wakeStartTime = 0;              // Track wake period start

  // Add timing control for sensor readings - now configurable
  unsigned long deviceStartTime = 0;
  unsigned long lastReadingTime = 0;
  unsigned long sensorRead_interval = 30000; // 30 seconds between readings (configurable)

  unsigned long SENSOR_STABILIZATION_TIME = 15000; // 15 seconds stabilization (configurable)
  bool deviceStabilized = false;
  unsigned long lastSensorRead = 0;
  unsigned long lastPublishTime = 0;
  unsigned long lastHTTPPublishTime = 0; // New timer for HTTP publishing
  uint32_t lastTimeSyncEpoch = 0;

  unsigned long currentTime = 0;
  unsigned long lastWiFiRetry = 0;

  bool wasConnected = false;
  String deviceID = DEVICE_ID; // Will be overridden by config (configurable)
  String idCode = IDCODE;      // Unique device identifier (configurable)

  // HTTP Publishing settings (configurable)
  bool httpPublishEnabled = true;
  unsigned long httpPublishInterval = 30000; // 30 seconds default

  bool sensorError = false;
  uint32_t lastErrorTime = 0;
  String lastErrorMessage;

  bool apAlwaysOn = false;

} state;