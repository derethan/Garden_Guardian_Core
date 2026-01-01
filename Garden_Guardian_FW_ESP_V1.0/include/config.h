#ifndef PINDEFINITIONS_H
#define PINDEFINITIONS_H

#define DEBUG_MODE false

// Device Information
#define DEVICE_ID "GG-"
#define IDCODE "00000001" // Unique device identifier

// Web Display Configuration Flags
#define TEMP_DISPLAY false
#define HUMIDITY_DISPLAY false
#define TDS_DISPLAY true
#define WATER_TEMP_DISPLAY false

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

#endif // PINDEFINITIONS_H