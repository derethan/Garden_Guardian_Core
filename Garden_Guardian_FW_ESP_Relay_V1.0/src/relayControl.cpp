#include "relayControl.h"

// Constructor
RelayControl::RelayControl(uint8_t relayPin, float hysteresis)
    : relayPin(relayPin), hysteresis(hysteresis), relayState(false), manualOverride(false) {}

void RelayControl::setManualOverride(bool override)
{
    manualOverride = override;
}

bool RelayControl::isManualOverride()
{
    return manualOverride;
}
// Initialize the relay
void RelayControl::initialize()
{
    pinMode(relayPin, OUTPUT);
    turnOff();
}

void RelayControl::turnOn()
{
    digitalWrite(relayPin, LOW);
    relayState = true;
}

void RelayControl::turnOff()
{
    digitalWrite(relayPin, HIGH);
    relayState = false;
}

bool RelayControl::isOn()
{
    return relayState;
}

// Set the relay based on timed intervals, for example, every x minutes turn on for y minutes and then turn off
void RelayControl::setRelayForTimedIntervals(int onInterval, int offInterval)
{
    if (manualOverride)
        return;

    // Get the current time
    unsigned long currentTime = millis();

    // Check if the relay is on
    if (relayState)
    {
        // Check if the relay has been on for the on interval
        if (currentTime - previousMillis >= onInterval * 60 * 1000)
        {
            // Turn off the relay
            turnOff();

            // Reset the previous millis
            previousMillis = currentTime;
        }
    }
    else
    {
        // Check if the relay has been off for the off interval
        if (currentTime - previousMillis >= offInterval * 60 * 1000 || previousMillis == 0)
        {
            // Turn on the relay
            turnOn();

            // Reset the previous millis
            previousMillis = currentTime;
        }
    }
}

// Set the relay based on the temperature
void RelayControl::setRelayforTemp(float temperature, float targetTemperature)
{
    if (manualOverride)
        return;

    if (relayState)
    {
        if (temperature >= targetTemperature)
        {
            turnOff();
        }
    }
    else
    {
        if (temperature <= targetTemperature - hysteresis)
        {
            turnOn();
        }
    }
}

// Set the relay based on the schedule
void RelayControl::setRelayForSchedule(int onHour, int offHour, String currentTime)
{
    if (manualOverride)
        return;

    int currentHour = currentTime.substring(0, 2).toInt();

    if (currentHour >= onHour && currentHour < offHour)
    {
        if (!relayState)
        {
            // Debugging: Print The time is between the on and off hours
            Serial.println("The time is between " + String(onHour) + " and " + String(offHour) + " hours");
            Serial.println("Turning on relay");

            turnOn();
        }
    }
    else
    {
        if (relayState)
        {
            // Debugging: Print The time is not between the on and off hours
            Serial.println("The time is not between " + String(onHour) + " and " + String(offHour) + " hours");
            Serial.println("Turning off relay");

            turnOff();
        }
    }
}

// Set the relay based on the error state
void RelayControl::setRelayForError(bool state)
{
    if (state)
    {
        turnOn();
    }
    else
    {
        turnOff();
    }
}

// Toggle the relay
void RelayControl::toggleRelay()
{
    if (relayState)
    {
        turnOff();
    }
    else
    {
        turnOn();
    }
}

// timer for relay check : every 15 seconds
void RelayControl::checkRelay(RelayControl &relay, float temperature, float targetTemperature)
{
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 15000)
    {
        previousMillis = currentMillis;
    }
}
