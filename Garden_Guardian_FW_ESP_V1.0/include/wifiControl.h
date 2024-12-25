
// #ifndef WIFICONTROL_H
// #define WIFICONTROL_H

// #include <Arduino.h>
// #include <WiFi.h>
// #include <esp_now.h>
// // #include <WiFiClientSecure.h>

// // MAC Address of the receiver
// extern uint8_t GGAddress[];


// typedef struct struct_message
// {
//     int id;
//     float temperature;
//     float humidity;
// } struct_message;

// class WifiControl
// {
// public:
//     // ssl wifi client
//     WifiControl(const char *ssid, const char *pass);
//     void connect();
//     void printStatus();
//     void modemStatus(int status);
//     void sendData(int id);
//     void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
//     void OnDataRecv(const uint8_t *mac, const uint8_t *data, int data_len);
//     void espConnect();
//     String getMacAddress();

// private:
//     const char *ssid;
//     const char *pass;
//     int status;
// };

// #endif // WIFICONTROL_H