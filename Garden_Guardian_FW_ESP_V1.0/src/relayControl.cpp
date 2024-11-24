#include "relayControl.h"

// Constructor
RelayControl::RelayControl(uint8_t relayPin, float hysteresis) : relayPin(relayPin), hysteresis(hysteresis), relayState(false) {}

// Initialize the relay
void RelayControl::initialize() {
    pinMode(relayPin, OUTPUT);
    turnOff();
}

void RelayControl::turnOn() {
    digitalWrite(relayPin, LOW);
    relayState = true;
}

void RelayControl::turnOff() {
    digitalWrite(relayPin, HIGH);
    relayState = false;
}

bool RelayControl::isOn() {
    return relayState;
}

void RelayControl::setRelayforTemp(float temperature, float targetTemperature) {
    if (relayState) {
        if (temperature >= targetTemperature) {
            turnOff();
        }
    } else {
        if (temperature <= targetTemperature - hysteresis) {
            turnOn();
        }
    }
}

// timer for relay check : every 15 seconds
void RelayControl::checkRelay(float temperature, float targetTemperature) {
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 15000) {
        previousMillis = currentMillis;
        // setRelay(temperature, targetTemperature);
    }
}