#ifndef GETTIME_H
#define GETTIME_H

#include <Arduino.h>

class TimeRetriever {
public:
    TimeRetriever(); // Constructor
    void initialize();
    String getCurrentTime();
    String getCurrentDate();
    String getTimestamp();

private:
    // Add any private members if needed
};

#endif // GETTIME_H
