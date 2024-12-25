// include this file in any other file that needs to use the pin definitions

#ifndef PINDEFINITIONS_H
#define PINDEFINITIONS_H

#define GG_MAIN_MAC {0xCC, 0xDB, 0xA7, 0x32, 0x24, 0xFC}


// Defined Relay pins
#define RELAY_PIN_LIGHTS 25
#define RELAY_PIN_HEATER_WATER_1 26
#define RELAY_PIN_HEATER_ROOM 27
#define RELAY_PIN_PUMP_WATER_1 14


#define LightSensor_Pin 25

#define SERIESRESISTOR 10000
#define NOMINAL_RESISTANCE 10000
#define NOMINAL_TEMPERATURE 25
#define BCOEFFICIENT 3950

// Defined DHT pins
#define DHTPIN1 2
#define DHTPIN2 1
#define DHTTYPE DHT11

// Define the initial target temperature
#define INITIAL_TEMP 20

#define TdsSensorPin A5
#define VREF 5.0   // analog reference voltage(Volt) of the ADC
#define SCOUNT 30  // sum of sample point

#define NTCPin A0

#define sensorArray_Size 100

#define PHPIN A1

#endif // PINDEFINITIONS_H