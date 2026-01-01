
#ifndef SERVERREQUEST_H
#define SERVERREQUEST_H

#include <Arduino.h>
#include <ArduinoHttpClient.h>

class ServerRequest {
public:
    ServerRequest(Client& client, const char* serverAddress, int serverPort);
    void makeGetRequest(const char* serverRoute);
    void postSensorData(const char* serverRoute, const String& postData);

private:
    HttpClient client;
};

#endif // SERVERREQUEST_H