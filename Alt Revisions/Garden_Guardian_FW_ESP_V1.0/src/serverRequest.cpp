
#include "serverRequest.h"


ServerRequest::ServerRequest(Client& client, const char* serverAddress, int serverPort) : client(client, serverAddress, serverPort) {}

void ServerRequest::makeGetRequest(const char* serverRoute) {
    client.stop();
    client.get(serverRoute);

    if (!client.connected()) {
        Serial.println("Failed to Connect to API Server");
        return;
    }

    unsigned long startTime = millis();
    while (!client.available()) {
        if (millis() - startTime > 5000) {
            Serial.println("Server Response Timeout");
            return;
        }
    }

    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

    if (statusCode > 0) {
        Serial.print("HTTP Response Status Code: ");
        Serial.println(statusCode);
        Serial.print("Response: ");
        Serial.println(response);
    } else {
        Serial.println("HTTP Request failed");
    }
}

void ServerRequest::postSensorData(const char* serverRoute, const String& postData) {
    client.beginRequest();
    client.post(serverRoute);

    if (!client.connected()) {
        client.stop();
        Serial.println("Failed to send Data");
        return;
    }

    client.sendHeader("Content-Type", "application/json");
    client.sendHeader("Content-Length", postData.length());
    client.beginBody();
    client.print(postData);
    client.endRequest();

    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

    if (statusCode > 0) {
        Serial.print("HTTP Response Status Code: ");
        Serial.println(statusCode);
        Serial.print("Response: ");
        Serial.println(response);
    } else {
        Serial.println("HTTP Request failed");
    }
}