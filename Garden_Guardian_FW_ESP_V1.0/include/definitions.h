// include this file in any other file that needs to use the pin definitions

#ifndef PINDEFINITIONS_H
#define PINDEFINITIONS_H

// Add pin definitions here

// Defined Relay pins
#define RELAY_PIN_1 25
#define RELAY_PIN_2 26
#define RELAY_PIN_3 27
#define RELAY_PIN_4 14


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