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
    void setRelayforTemp(float temperature, float targetTemperature);
    void checkRelay(float temperature, float targetTemperature);

private:
    uint8_t relayPin;
    float hysteresis;
    bool relayState;
};

#endif // RELAYCONTROL_H
