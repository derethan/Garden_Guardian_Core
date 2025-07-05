#ifndef LATESTREADINGS_H
#define LATESTREADINGS_H

// Structure to store latest sensor readings for web display
struct LatestReadings
{
    float temperature = NAN;
    float humidity = NAN;
    unsigned long temperatureTimestamp = 0;
    unsigned long humidityTimestamp = 0;
    int temperatureStatus = 500;  // Default to error
    int humidityStatus = 500;     // Default to error
    bool hasValidData = false;
};

#endif // LATESTREADINGS_H
