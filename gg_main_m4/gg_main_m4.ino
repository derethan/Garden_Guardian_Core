#include <RPC.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>

#include "arduino_secrets.h" 

//Set Connection Info
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
const char* serverAddress = "192.168.0.150";
const int serverPort = 3000;
const char* serverRoute = "/store";
const char* serverRouteGet = "/retrieve";
int status = WL_IDLE_STATUS;
WiFiClient client;


void setup() {

  RPC.begin();
}


void loop() {


  RPC.println("Message From M4 ");


delay (10000);
}
