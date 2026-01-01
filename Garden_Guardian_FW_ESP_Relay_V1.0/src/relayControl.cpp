#include "relayControl.h"
#include "config.h"
#include "base/sysLogs.h"

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
    turnOn(); // This sets the physical relay ON and relayState = true

    SysLogs::logInfo("RELAY", "Relay initialized - Pin: " + String(relayPin) + ", State: " + String(relayState ? "ON" : "OFF"));
}

void RelayControl::turnOff()
{
    digitalWrite(relayPin, HIGH);
    relayState = false;
}

void RelayControl::turnOn()
{
    digitalWrite(relayPin, LOW);
    relayState = true;
}

bool RelayControl::isOn()
{
    return relayState;
}

// Set the relay based on timed intervals, for example, every x minutes turn on for y minutes and then turn off
void RelayControl::setRelayForTimedIntervals(int onInterval, int offInterval)
{
    if (manualOverride)
    {
        SysLogs::logInfo("RELAY", "Manual override is active, skipping timed interval check.");
        return;
    }

    // Get the current time
    unsigned long currentTime = millis();

    // Check if the previous millis is 0
    if (previousMillis == 0)
    {
        previousMillis = currentTime;
        SysLogs::logInfo("RELAY", "Initializing previousMillis to current time.");
    }

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
            // Debug
            SysLogs::logInfo("RELAY", "Turning off relay after on interval.");
        }
        else
        {
            SysLogs::logInfo("RELAY", "Relay is on, waiting for on interval to complete.");
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
            // Debug
            SysLogs::logInfo("RELAY", "Turning on relay after off interval.");
        }
        else
        {
            SysLogs::logInfo("RELAY", "Relay is off, waiting for off interval to complete.");
        }
    }
}

// Set the relay based on the temperature
void RelayControl::setRelayforTemp(float temperature, float targetTemperature)
{
    // Enhanced debugging - always print temperature check info
    SysLogs::logInfo("RELAY", "Temperature Check - Current: " + String(temperature) + "°C, Target: " + String(targetTemperature) + "°C, Relay State: " + String(relayState ? "ON" : "OFF") + ", Manual Override: " + String(manualOverride ? "ACTIVE" : "INACTIVE"));
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
    // Enhanced debugging
SysLogs::logInfo("RELAY", "Schedule Check - On Hour: " + String(onHour) + ", Off Hour: " + String(offHour) + ", Current Time: " + currentTime + ", Relay State: " + String(relayState ? "ON" : "OFF") + ", Manual Override: " + String(manualOverride ? "ACTIVE" : "INACTIVE"));

    if (manualOverride)
    {
        SysLogs::logInfo("RELAY", "Manual override is active - skipping schedule control");
        SysLogs::logInfo("RELAY", "---------------------------");
        return;
    }

    // Current time comes in like 14:58:59, extract parameters
    int currentHour = currentTime.substring(0, 2).toInt();

    if (currentHour >= onHour && currentHour < offHour)
    {
        SysLogs::logInfo("RELAY", "Time is within schedule period (" + String(onHour) + ":00 - " + String(offHour) + ":00)");
        if (!relayState)
        {
            SysLogs::logInfo("RELAY", "Relay is OFF - turning ON for schedule");
            turnOn();
        }
        else
        {
            SysLogs::logInfo("RELAY", "Relay is already ON - no action needed");
        }
    }
    else
    {
        SysLogs::logInfo("RELAY", "Time is outside schedule period (" + String(onHour) + ":00 - " + String(offHour) + ":00)");
        if (relayState)
        {
            SysLogs::logInfo("RELAY", "Relay is ON - turning OFF (outside schedule)");
            turnOff();
        }
        else
        {
            SysLogs::logInfo("RELAY", "Relay is already OFF - no action needed");
        }
    }
    SysLogs::logInfo("RELAY", "Final Relay State: " + String(relayState ? "ON" : "OFF"));
    SysLogs::logInfo("RELAY", "---------------------------");
}

/*******
 * Auto Feeding System Control
 * This function controls the nutrient feeding system based on TDS values.
 * - Feeds for 5 seconds when TDS is below target
 * - Waits for stabilization period before allowing next feeding cycle
 **********************************************************************************************/
void RelayControl::setAutoFeedingSystem(float tdsValue, float targetTDS, unsigned long stabilizationDelay)
{
    if (manualOverride)
    {
        Serial.println("Manual override active - skipping auto-feeding control");
        return;
    }

    unsigned long currentTime = millis();

    // Enhanced debugging
    Serial.println("--- Auto Feeding System Check ---");
    Serial.println("Current TDS: " + String(tdsValue) + " PPM");
    Serial.println("Target TDS: " + String(targetTDS) + " PPM");
    Serial.println("Currently Feeding: " + String(currentlyFeeding ? "YES" : "NO"));
    Serial.println("Relay State: " + String(relayState ? "ON" : "OFF"));

    // Handle active feeding cycle
    if (currentlyFeeding)
    {
        unsigned long feedingElapsed = currentTime - feedingStartTime;
        Serial.println("Feeding in progress - Elapsed: " + String(feedingElapsed) + "ms / " + String(FEEDING_DURATION) + "ms");

        if (feedingElapsed >= FEEDING_DURATION)
        {
            // Stop feeding after 5 seconds
            turnOff();
            currentlyFeeding = false;
            lastFeedingTime = currentTime;
            Serial.println("Feeding cycle completed - waiting for stabilization");
        }
        Serial.println("----------------------------------");
        return;
    }

    // Check if we're in stabilization period
    if (lastFeedingTime > 0)
    {
        unsigned long timeSinceLastFeeding = currentTime - lastFeedingTime;
        Serial.println("Time since last feeding: " + String(timeSinceLastFeeding / 1000) + "s / " + String(stabilizationDelay / 1000) + "s");

        if (timeSinceLastFeeding < stabilizationDelay)
        {
            Serial.println("Still in stabilization period - no feeding allowed");
            Serial.println("----------------------------------");
            return;
        }
        else
        {
            Serial.println("Stabilization period complete - ready for next feeding if needed");
        }
    }

    // Check if feeding is needed
    float feedingThreshold = targetTDS;
    Serial.println("Feeding threshold: " + String(feedingThreshold) + " PPM");
    Serial.println("TDS comparison: " + String(tdsValue) + " < " + String(feedingThreshold) + " = " + String(tdsValue < feedingThreshold ? "TRUE" : "FALSE"));

    if (tdsValue < feedingThreshold)
    {
        Serial.println("TDS below target (with hysteresis) - starting feeding cycle");
        turnOn();
        currentlyFeeding = true;
        feedingStartTime = currentTime;
        Serial.println("Feeding started - will run for " + String(FEEDING_DURATION / 1000) + " seconds");
    }
    else
    {
        Serial.println("TDS within acceptable range - no feeding needed");
        // Ensure relay is off if we're not feeding
        if (relayState)
        {
            turnOff();
        }
    }

    Serial.println("----------------------------------");
}