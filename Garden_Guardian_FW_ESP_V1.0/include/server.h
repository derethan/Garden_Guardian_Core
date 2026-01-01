#ifndef SERVER_H
#define SERVER_H

#include "secrets.h" // Include your secrets file for API server and port

struct serverData
{
    const char *address = SECRET_API_SERVER;
    const int port = SECRET_PORT;
    const char *apiPostRoute = "/sensors/sendData";
    const char *apiGetRoute = "/sensors/retrieve";
    const char *apiAckRoute = "/sensors/ack";
    const char *test = "/sensors/testconnection";
    const char *ping = "/sensors/ping";
};

#endif // SERVER_H