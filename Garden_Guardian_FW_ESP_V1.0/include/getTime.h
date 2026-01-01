#ifndef GETTIME_H
#define GETTIME_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class TimeRetriever {
public:
    TimeRetriever(); // Constructor
    void initialize();
    String getCurrentTime();
    String getCurrentDate();
    String getTimestamp();
    unsigned long getUnixTime();

private:
    // Add any private members if needed
};

#endif // GETTIME_H
