/**
 * @file mqttConnection.cpp
 * @brief MQTT connection management for AWS IoT Core integration
 * 
 * Handles MQTT client initialization, connection management, message publishing,
 * and incoming message handling for AWS IoT Core. Includes configuration update
 * handling and device management features.
 */

#include "mqttConnection.h"
#include "NetworkConnections.h"
#include "base/sysLogs.h"

// ===== Private Static Members =====
static WiFiClient wifiClient; // WiFi client for network connection
WiFiClientSecure net = WiFiClientSecure();
MQTTClient mqttClient = MQTTClient(256);
Preferences MqttConnection::preferences;
NetworkConnections netConn; // Create an instance

static bool isMQTTConnected = false; // Track connection status
static const char broker[] = AWS_IOT_ENDPOINT;
String MqttConnection::deviceID = "";

/**
 * @brief Initialize MQTT client with AWS IoT credentials
 * @param deviceID The unique device identifier for this MQTT client
 * 
 * Configures the secure WiFi client with AWS IoT certificates and sets up
 * the MQTT client with the appropriate endpoint and message handler.
 */
void MqttConnection::initializeMQTT(const char *deviceID)
{
    MqttConnection::deviceID = String(deviceID); // Store the deviceID

    // Configure WiFiClientSecure to use the AWS IoT device credentials
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // Connect to the MQTT broker on the AWS endpoint we defined earlier
    mqttClient.begin(AWS_IOT_ENDPOINT, AWS_IOT_PORT, net);

    // Create a handler for incoming messages
    mqttClient.onMessage(messageHandler);
}

/**
 * @brief Attempt to connect to the AWS IoT MQTT broker
 * 
 * Performs connection attempts with retry logic and timeout constraints.
 * Subscribes to configuration topic upon successful connection.
 */
void MqttConnection::connectMQTT()
{
    SysLogs::logInfo("MQTT", "Connecting to AWS IoT broker: " + String(broker));

    const int maxAttempts = 5;
    const unsigned long maxDuration = 60000;
    int attempts = 0;
    unsigned long startTime = millis();

    while (!mqttClient.connect(deviceID.c_str(), false))
    {
        SysLogs::print(".");
        delay(100);
        attempts++;

        if (attempts >= maxAttempts || (millis() - startTime) >= maxDuration)
        {
            SysLogs::logError("Failed to connect to AWS IoT Core within the allowed attempts/duration.");
            isMQTTConnected = false;
            return;
        }
    }

    isMQTTConnected = true;
    SysLogs::logSuccess("MQTT", "Connected to AWS IoT Core!");

    // Subscribe to incoming message topic
    SysLogs::logInfo("MQTT", "Subscribing to configuration topic: " + String(AWS_IOT_CONFIGURATION_TOPIC) + deviceID);
    mqttClient.subscribe(String(AWS_IOT_CONFIGURATION_TOPIC) + deviceID);
}

/**
 * @brief Check MQTT connection status and reconnect if necessary
 * 
 * Maintains the MQTT connection by checking connectivity and calling
 * the MQTT client loop. Reconnects if the connection is lost.
 */
void MqttConnection::checkConnection()
{
    if (!mqttClient.connected())
    {
        connectMQTT();
    }
    mqttClient.loop(); // Keep the MQTT connection alive
}

/**
 * @brief Disconnect from the MQTT broker
 * 
 * Gracefully disconnects from the MQTT broker if currently connected.
 */
void MqttConnection::disconnect()
{
    if (mqttClient.connected())
    {
        SysLogs::logInfo("MQTT", "Disconnecting from MQTT broker...");
        mqttClient.disconnect();
        delay(100);
        SysLogs::logInfo("MQTT", "Disconnected from MQTT broker");
    }
}

/**
 * @brief Check if currently connected to the MQTT broker
 * @return true if connected, false otherwise
 */
bool MqttConnection::isConnected()
{
    return mqttClient.connected();
}

/**
 * @brief Publish a message to the AWS IoT MQTT broker
 * @param message The message string to publish
 * @return true if published successfully, false otherwise
 */
bool MqttConnection::publishMessage(String message)
{
    SysLogs::logDebug("MQTT", "Publishing message");

    if (mqttClient.publish(AWS_IOT_PUBLISH_TOPIC, message))
    {
        SysLogs::logDebug("MQTT", "Message sent successfully");
        return true;
    }

    SysLogs::logError("Failed to send MQTT message");
    return false;
}

/**
 * @brief Handle incoming MQTT messages from AWS IoT Core
 * @param topic The MQTT topic the message was received on
 * @param payload The message payload content
 * 
 * Routes incoming messages to appropriate handlers based on topic.
 */
void MqttConnection::messageHandler(String &topic, String &payload)
{
    SysLogs::logInfo("MQTT", "Received message:");
    SysLogs::logInfo("MQTT", "- topic: " + topic);
    SysLogs::logInfo("MQTT", "- payload:");
    SysLogs::println(payload);

    // Process the incoming data as json object, Then conditionally handle based on topic
    if (topic == AWS_IOT_CONFIGURATION_TOPIC + deviceID)
    {
        handleConfigurationTopic(payload);
    }
}

/**
 * @brief Handle configuration update messages from AWS IoT
 * @param payload The JSON configuration payload
 * 
 * Processes configuration updates including collection intervals, publish intervals,
 * and WiFi settings. Sends acknowledgments for successful updates.
 */
void MqttConnection::handleConfigurationTopic(String &payload)
{
    // Verify we have a valid payload
    if (payload.length() == 0)
    {
        SysLogs::logError("Empty calibration payload");
        return;
    }

    // Parse the JSON payload
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    // Check for parsing errors
    if (error)
    {
        SysLogs::logError("Error parsing JSON: " + String(error.c_str()));
        return;
    }

    // Validate required fields
    if (doc["device_id"].isNull() || doc["settings"].isNull())
    {
        errorLog("Invalid configuration: missing required fields", true);
        return;
    }

    // Extract device ID from message
    String messageDeviceId = doc["device_id"].as<String>();

    // Verify the message is intended for this device
    if (messageDeviceId != deviceID)
    {
        errorLog("Configuration message not for this device", true);
        return;
    }

    // Check for collection_interval in settings
    if (!doc["settings"]["collection_interval"].isNull())
    {
        // Extract the collection interval in ms
        unsigned long interval = doc["settings"]["collection_interval"].as<unsigned long>();

        // Update the collection interval
        state.sensorRead_interval = interval;
        debug("Collection interval update requested: " + String(interval / 1000) + " seconds", true);

        // Send acknowledgment
        String ackMessage = "{\"device_id\":\"" + deviceID +
                            "\",\"status\":\"success\"," +
                            "\"config\":{\"collection_interval\":" + String(interval) + "}}";

        mqttClient.publish(AWS_IOT_CONFIG_ACK_TOPIC, ackMessage);

        // Save to NVS as a backup
        preferences.begin("config", false); //
        preferences.putString("cInterval", String(interval));
        preferences.end();
    }

    else if (!doc["settings"]["publish_interval"].isNull())
    {
        // Extract the collection interval in seconds
        unsigned long interval = doc["settings"]["publish_interval"].as<unsigned long>();

        // Update the collection interval
        state.httpPublishInterval = interval;
        debug("Collection interval update requested: " + String(interval) + " seconds", true);

        // Send acknowledgment
        String ackMessage = "{\"device_id\":\"" + deviceID +
                            "\",\"status\":\"success\"," +
                            "\"config\":{\"publish_interval\":" + String(interval) + "}}";

        mqttClient.publish(AWS_IOT_CONFIG_ACK_TOPIC, ackMessage);

        // Save to NVS as a backup
        preferences.begin("config", false); //
        preferences.putString("pInterval", String(interval));
        preferences.end();
    }

    // Check for Wifi SSID and Password in settings
    else if (!doc["settings"]["wifi_settings"].isNull())
    {
        // Extract the WiFi settings
        JsonObject wifiSettings = doc["settings"]["wifi_settings"];

        // Check for required fields
        if (wifiSettings["ssid"].isNull() || wifiSettings["password"].isNull())
        {
            debug("Invalid WiFi settings: missing required fields", true);
            return;
        }

        // Extract the SSID and password
        String ssid = wifiSettings["ssid"].as<String>();
        String password = wifiSettings["password"].as<String>();

        // Save Wifi Credentials
        netConn.saveWiFiCredentials(ssid, password); // Call the method

        // Send acknowledgment
        String ackMessage = "{\"device_id\":\"" + deviceID +
                            "\",\"status\":\"success\"," +
                            "\"config\":{\"wifi_settings\":{\"ssid\":\"" + ssid + "\"}}}";

        mqttClient.publish(AWS_IOT_CONFIG_ACK_TOPIC, ackMessage);

        debug("WiFi settings updated: " + ssid, true);

        // Restart the device to apply the new settings
        delay(1000);
        ESP.restart();
    }
}

/**
 * @brief Get current timestamp as formatted string
 * @return Timestamp string in HH:MM:SS.mmm format
 */
String MqttConnection::getTimestamp()
{
    unsigned long now = NetworkConnections::getTime() * 1000;
    unsigned long seconds = now / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    String timestamp = String(hours) + ":" +
                       String(minutes % 60) + ":" +
                       String(seconds % 60) + "." +
                       String(now % 1000);
    return timestamp;
}

/**
 * @brief Log a debug message and optionally publish to AWS IoT
 * @param message The debug message to log
 * @param publishToAWS If true, publish the message to AWS IoT debug topic
 * 
 * Creates a JSON-formatted debug message with device ID and timestamp.
 */
void MqttConnection::debug(const String &message, bool publishToAWS)
{

    unsigned long now = NetworkConnections::getTime() * 1000;
    // Create JSON structure
    String jsonMessage = "{";
    jsonMessage += "\"device_id\":\"" + deviceID + "\",";
    jsonMessage += "\"timestamp\":\"" + String(now) + "\",";
    jsonMessage += "\"message\":\"" + message + "\"";
    jsonMessage += "}";

    // Always print to log
    SysLogs::logDebug("MQTT", jsonMessage);

    // Publish to AWS if requested and connected
    if (publishToAWS && mqttClient.connected())
    {
        // Publish the message to the topic with device ID
        mqttClient.publish(AWS_IOT_DEBUG_TOPIC + deviceID, jsonMessage);
    }
}

/**
 * @brief Log an error message and optionally publish to AWS IoT
 * @param message The error message to log
 * @param publishToAWS If true, publish the error to AWS IoT error topic
 * 
 * Creates a JSON-formatted error message with device ID and timestamp.
 */
void MqttConnection::errorLog(const String &message, bool publishToAWS)
{
    unsigned long now = NetworkConnections::getTime() * 1000;
    // Create JSON structure
    String jsonMessage = "{";
    jsonMessage += "\"device_id\":\"" + deviceID + "\",";
    jsonMessage += "\"timestamp\":\"" + String(now) + "\",";
    jsonMessage += "\"error\":\"" + message + "\"";
    jsonMessage += "}";

    // Always log the error
    SysLogs::logError(jsonMessage);

    // Publish to AWS if requested and connected
    if (publishToAWS && mqttClient.connected())
    {
        // Publish the error message to the error topic with device ID
        mqttClient.publish(AWS_IOT_ERROR_TOPIC + deviceID, jsonMessage);
    }
}