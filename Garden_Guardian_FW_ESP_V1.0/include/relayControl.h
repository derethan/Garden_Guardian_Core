#ifndef RELAYCONTROL_H
#define RELAYCONTROL_H

#include <Arduino.h>
class RelayControl
{
public:
    RelayControl(uint8_t relayPin, float hysteresis = 1.0);
    void initialize();
    void turnOn();
    void turnOff();
    bool isOn();
    void setManualOverride(bool override);
    bool isManualOverride();
    void setRelayForTimedIntervals(int onInterval, int offInterval);
    void setRelayforTemp(float temperature, float targetTemperature);
    void setRelayForSchedule(int onHour, int offHour, String currentTime);
    
    // Auto-feeding system for nutrient control
    void setAutoFeedingSystem(float tdsValue, float targetTDS, unsigned long stabilizationDelay = 300000);

private:
    uint8_t relayPin;
    float hysteresis;
    bool relayState;
    bool manualOverride;
    unsigned long previousMillis = 0;
    
    // Auto-feeding system variables
    unsigned long feedingStartTime = 0;
    unsigned long lastFeedingTime = 0;
    bool currentlyFeeding = false;
    static const unsigned long FEEDING_DURATION = 5000; // 5 seconds
};

#endif // RELAYCONTROL_H
