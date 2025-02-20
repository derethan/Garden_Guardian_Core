#ifndef RELAYCONTROL_H
#define RELAYCONTROL_H

#include <Arduino.h>
class RelayControl
{
public:
    RelayControl(uint8_t relayPin, float hysteresis = 5.0);
    void initialize();
    void turnOn();
    void turnOff();
    bool isOn();
    void setRelayForTimedIntervals(int onInterval, int offInterval);
    void setRelayforTemp(float temperature, float targetTemperature);
    void checkRelay(RelayControl &relay, float temperature, float targetTemperature);
    void setRelayForSchedule(int onHour, int offHour, String currentTime);
    void setRelayForError(bool state);
    void toggleRelay();
    void setManualOverride(bool override);
    bool isManualOverride();

private:
    uint8_t relayPin;
    float hysteresis;
    bool relayState;
    unsigned long previousMillis;
    bool manualOverride;
};

#endif // RELAYCONTROL_H
