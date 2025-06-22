#ifndef TDSSENSOR_H
#define TDSSENSOR_H

#include <Arduino.h>

class TDSSensor {
public:
    TDSSensor(uint8_t pin, float vref = 5.0, int scount = 30);
    float read(float temperature = 25.0);

private:
    uint8_t tdsPin;
    float vref;
    int scount;
    int getMedianNum(int bArray[], int iFilterLen);
    float getAverageNum(int bArray[], int iFilterLen);
};

#endif // TDSSENSOR_H
