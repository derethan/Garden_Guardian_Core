#ifndef RELAYCONTROL_H
#define RELAYCONTROL_H

#include <Arduino.h>
#include "getTime.h" // Include the header for time retrieval
class RelayControl
{
public:
    RelayControl(uint8_t relayPin, float hysteresis = 5.0);
    void initialize();
    void turnOn();
    void turnOff();
    bool isOn();
    void setRelayForTimedIntervals();
    void setRelayforTemp(float temperature, float targetTemperature);
    void checkRelay(RelayControl &relay, float temperature, float targetTemperature);
    void setRelayForSchedule(TimeRetriever &timeRetriever);
    void setRelayForError(bool state);
    void toggleRelay();

private:
    uint8_t relayPin;
    float hysteresis;
    bool relayState;
    unsigned long previousMillis;
};

#endif // RELAYCONTROL_H
