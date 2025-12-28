#ifndef MQTTCONNECTION_H
#define MQTTCONNECTION_H

// Sensitive Data
#include "aws_secrets.h"
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "state.h"
#include <Preferences.h>

extern unsigned long last_MQTTPublish;

class MqttConnection
{

private:
    static String deviceID; // Add static deviceID member
    static Preferences preferences;  

    // Add timestamp formatting helper
    static String getTimestamp();
    static void messageHandler(String &topic, String &payload);
    static void handleConfigurationTopic(String &payload);

public:
    void initializeMQTT(const char *deviceID);
    void connectMQTT();
    bool publishMessage(String message);
    void checkConnection();
    bool isConnected();
    void disconnect(); // New method to properly disconnect MQTT before sleep

    // Debug logging method
    static void debug(const String &message, bool publishToAWS = true);
    // Error logging method
    static void errorLog(const String &message, bool publishToAWS = true);
};

#endif // MQTTCONNECTION_H