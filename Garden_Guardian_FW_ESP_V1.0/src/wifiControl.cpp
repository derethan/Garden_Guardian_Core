
// #include "wifiControl.h"

// struct_message myData;
// uint8_t GGAddress[] = {0xCC, 0xDB, 0xA7, 0x32, 0x24, 0xFC};

// // Constructor
// WifiControl::WifiControl(const char *ssid, const char *pass) : ssid(ssid), pass(pass), status(WL_IDLE_STATUS)
// {
// }

// void WifiControl::OnDataSent(const uint8_t *macAddr, esp_now_send_status_t status)
// {
//     Serial.print("Delivery Status: ");
//     Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
// }

// void WifiControl::OnDataRecv(const uint8_t *mac, const uint8_t *data, int data_len)
// {
//     Serial.print("Data received: ");
//     struct_message *message = (struct_message *)data;
//     Serial.println(message->id);
// }

// void WifiControl::espConnect()
// {
//     // Set device as a Wi-Fi Station
//     WiFi.mode(WIFI_STA);

//     // Init ESP-NOW
//     if (esp_now_init() != ESP_OK)
//     {
//         Serial.println("Error initializing ESP-NOW");
//         return;
//     }

//     // Register peer
//     esp_now_peer_info_t peerInfo;
//     memcpy(peerInfo.peer_addr, GGAddress, 6);
//     peerInfo.channel = 0;
//     peerInfo.encrypt = false;

//     // Add peer
//     if (esp_now_add_peer(&peerInfo) != ESP_OK)
//     {
//         Serial.println("Error adding peer");
//         return;
//     }

//     // // Register callback function
//     // esp_now_register_send_cb(OnDataSent);
//     // esp_now_register_recv_cb(OnDataRecv);
// }

// void WifiControl::sendData(int id)
// {
//     // Set values to send
//     myData.id = id;

//     // Send message via ESP-NOW
//     esp_err_t result = esp_now_send(GGAddress, (uint8_t *)&myData, sizeof(myData));
//     Serial.print("Sending: ");
//     Serial.println(result == ESP_OK ? "Success" : "Fail");
// }

// // Function to return the Mac Address of the ESP32
// String WifiControl::getMacAddress()
// {
//     uint8_t baseMac[6];
//     esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
//     char baseMacChr[18] = {0};
//     snprintf(baseMacChr, sizeof(baseMacChr), "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
//     return String(baseMacChr);
// }

// void WifiControl::connect()
// {
//     WiFi.begin(ssid, pass);

//     Serial.println("Attempting to connect to Wi-Fi...");

//     Serial.print("Connecting...");
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(1000);
//         Serial.print(".");
//     }

//     Serial.println("Connected to Wi-Fi");
//     printStatus();
// }

// void WifiControl::printStatus()
// {
//     int status = WiFi.status();

//     modemStatus(status);

//     Serial.print("SSID: ");
//     Serial.println(WiFi.SSID());
//     IPAddress ip = WiFi.localIP();
//     Serial.print("IP Address: ");
//     Serial.println(ip);
//     long rssi = WiFi.RSSI();
//     Serial.print("Signal Strength (RSSI): ");
//     Serial.print(rssi);
//     Serial.println(" dBm");
// }

// void WifiControl::modemStatus(int status)
// {
//     switch (status)
//     {
//     case WL_NO_SHIELD:
//         Serial.println("No Wi-Fi shield detected");
//         break;
//     case WL_IDLE_STATUS:
//         Serial.println("Idle status");
//         break;
//     case WL_NO_SSID_AVAIL:
//         Serial.println("No SSID available");
//         break;
//     case WL_SCAN_COMPLETED:
//         Serial.println("Scan completed");
//         break;
//     case WL_CONNECTED:
//         Serial.println("Connected to Wi-Fi");
//         break;
//     case WL_CONNECT_FAILED:
//         Serial.println("Connection failed");
//         break;
//     case WL_CONNECTION_LOST:
//         Serial.println("Connection lost");
//         break;
//     case WL_DISCONNECTED:
//         Serial.println("Disconnected");
//         break;
//     }
// }