#include "tdsSensor.h"
#include "config.h"

TDSSensor::TDSSensor(uint8_t pin, float vref, int scount)
    : tdsPin(pin), vref(vref), scount(scount) {}

float TDSSensor::read(float temperature)
{
#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Starting TDS sensor reading...");
#endif
    // Temperature validation - ensure it's in reasonable range (0-50°C)
    float validatedTemperature = temperature;
    if (temperature <= 0.0 || temperature > 50.0) // Changed condition to handle exactly 0.0°C
    {
#if DEBUG_MODE
        Serial.println("[TDS DEBUG] Temperature out of range or invalid (" + String(temperature) + "°C), defaulting to 25.0°C");
#endif
        validatedTemperature = 25.0;
    }
    else
    {
#if DEBUG_MODE
        Serial.println("[TDS DEBUG] Using temperature: " + String(validatedTemperature) + "°C");
#endif
    }

    int *analogBuffer = new int[scount];
    int *analogBufferTemp = new int[scount];
    float averageVoltage = 0, tdsValue = 0;

#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Starting TDS reading with " + String(scount) + " samples on pin " + String(tdsPin));
#endif

    // Collect analog readings
    for (int i = 0; i < scount; i++)
    {
        analogBuffer[i] = analogRead(tdsPin);
#if DEBUG_MODE
        if (i % 10 == 0)
        { // Print every 10th reading to avoid spam
            Serial.println("[TDS DEBUG] Sample " + String(i) + ": " + String(analogBuffer[i]));
        }
#endif
        delay(100);
    }
    // Copy buffer for median calculation
    for (int i = 0; i < scount; i++)
    {
        analogBufferTemp[i] = analogBuffer[i];
    }

    // Calculate average voltage using median
    int medianReading = getMedianNum(analogBufferTemp, scount);
    averageVoltage = medianReading * vref / 4095.0; // ESP32 has 12-bit ADC (0-4095)
#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Median ADC reading: " + String(medianReading));
    Serial.println("[TDS DEBUG] Average voltage: " + String(averageVoltage) + "V (Vref: " + String(vref) + "V)");
#endif

    // Temperature compensation
    float compensationCoefficient = 1.0 + 0.02 * (validatedTemperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;
#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Compensation coefficient: " + String(compensationCoefficient));
    Serial.println("[TDS DEBUG] Compensated voltage: " + String(compensationVoltage) + "V");
#endif

    // Calculate TDS value using polynomial formula
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;

#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Final TDS value: " + String(tdsValue) + " ppm");
#endif
    delete[] analogBuffer;
    delete[] analogBufferTemp;
    return tdsValue;
}

int TDSSensor::getMedianNum(int bArray[], int iFilterLen)
{
#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Calculating median from " + String(iFilterLen) + " values");
#endif
    int *bTab = new int[iFilterLen];
    for (int i = 0; i < iFilterLen; i++)
        bTab[i] = bArray[i];

    // Find min and max values for debugging
    int minVal = bTab[0], maxVal = bTab[0];
    for (int i = 1; i < iFilterLen; i++)
    {
        if (bTab[i] < minVal)
            minVal = bTab[i];
        if (bTab[i] > maxVal)
            maxVal = bTab[i];
    }
#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Raw readings range: " + String(minVal) + " to " + String(maxVal));
#endif

    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++)
    {
        for (i = 0; i < iFilterLen - j - 1; i++)
        {
            if (bTab[i] > bTab[i + 1])
            {
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

#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Median value: " + String(bTemp));
#endif
    delete[] bTab;
    return bTemp;
}

float TDSSensor::getAverageNum(int bArray[], int iFilterLen)
{
#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Calculating average from " + String(iFilterLen) + " values");
#endif
    long sum = 0;
    for (int i = 0; i < iFilterLen; i++)
    {
        sum += bArray[i];
    }
    float average = (float)sum / iFilterLen;
#if DEBUG_MODE
    Serial.println("[TDS DEBUG] Sum: " + String(sum) + ", Average: " + String(average));
#endif
    return average;
}
