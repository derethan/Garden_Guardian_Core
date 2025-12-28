#ifndef CONFIG_H
#define CONFIG_H

// Debug mode - now a runtime variable instead of compile-time constant
// Declared in state.h and defined in main.cpp
extern bool DEBUG_MODE;

// Serial Configuration Mode Settings
#define SERIAL_ACCESS_PASSWORD "menu" // Default password to enter SERIAL_MODE
#define SERIAL_TIMEOUT 300000 // 5 minutes timeout for serial mode (milliseconds)

// Device Information
#define DEVICE_ID "GH_RELAY-"
#define IDCODE "Z9C5ACF9" // Unique device identifier
#define FIRMWARE_VERSION "1.0.0"
#define BANNER_TEXT "Garden Guardian Relay Controller V" FIRMWARE_VERSION

// pin definitions
#define DHTPIN 22     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)

// Web Display Configuration Flags
#define TEMP_DISPLAY true
#define HUMIDITY_DISPLAY true
#define TDS_DISPLAY false
#define WATER_TEMP_DISPLAY false
#define RELAY_STATUS_DISPLAY false

// Defined Relay pins
#define RELAY1_PIN 26
#define RELAY2_PIN 25
#define RELAY3_PIN 27
#define RELAY4_PIN 33

// DS18B20 Temperature Sensor
#define TEMP_SENSOR_PIN 23

// TDS Sensor
#define VREF 3.3  // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point

#define TDS_SENSOR_PIN 36
#define TDS_CTRL_PIN 13

#define sensorArray_Size 100


#endif // CONFIG_H