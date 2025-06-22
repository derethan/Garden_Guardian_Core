#include "tdsSensor.h"

TDSSensor::TDSSensor(uint8_t pin, float vref, int scount)
    : tdsPin(pin), vref(vref), scount(scount) {}

float TDSSensor::read(float temperature) {
    int *analogBuffer = new int[scount];
    int *analogBufferTemp = new int[scount];
    float averageVoltage = 0, tdsValue = 0;

    for (int i = 0; i < scount; i++) {
        analogBuffer[i] = analogRead(tdsPin);
        delay(100);
    }
    for (int i = 0; i < scount; i++) {
        analogBufferTemp[i] = analogBuffer[i];
    }
    averageVoltage = getMedianNum(analogBufferTemp, scount) * vref / 1024.0;
    // averageVoltage = getAverageNum(analogBuffer, scount) * vref / 1024.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
                - 255.86 * compensationVoltage * compensationVoltage
                + 857.39 * compensationVoltage) * 0.5;
    delete[] analogBuffer;
    delete[] analogBufferTemp;
    return tdsValue;
}

int TDSSensor::getMedianNum(int bArray[], int iFilterLen) {
    int *bTab = new int[iFilterLen];
    for (int i = 0; i < iFilterLen; i++)
        bTab[i] = bArray[i];
    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++) {
        for (i = 0; i < iFilterLen - j - 1; i++) {
            if (bTab[i] > bTab[i + 1]) {
                bTemp = bTab[i];
                bTab[i] = bTab[i + 1];
                bTab[i + 1] = bTemp;
            }
        }
    }
    if ((iFilterLen & 1) > 0)
        bTemp = bTab[(iFilterLen - 1) / 2];
    else
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    delete[] bTab;
    return bTemp;
}

float TDSSensor::getAverageNum(int bArray[], int iFilterLen) {
    long sum = 0;
    for (int i = 0; i < iFilterLen; i++) {
        sum += bArray[i];
    }
    return (float)sum / iFilterLen;
}
