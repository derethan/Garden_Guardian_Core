/*****************************************
*  Imported Libraries and files
*****************************************/
//Arduino Libraries
#include <Arduino.h>

#include <SPI.h>
#include <RPC.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <math.h>

#include "pitches.h"


//Connections
#include <DHT.h>
#include <microDS18B20.h>

//LCD Display
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);  // I2C address 0x27

//Wire Inputs
#include <Wire.h>

//Sensitive Data
#include "arduino_secrets.h"

//import Directory Files
#include "custom_char.h"
#include "lcd_functions.h"
#include "relay_control.h"
#include "buzzer_functions.h"
#include "getTime.h"

/*****************************************
*   Communications for WIFI and Server
*****************************************/
//Connection Info
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
WiFiClient wifi;


const char* serverAddress = SECRET_API_SERVER;
const int serverPort = SECRET_PORT;
const char* serverRoute = "/sensors/sendData";
const char* serverRouteGet = "/sensors/retrieve";
const char* serverTest = "/sensors/testconnection";
const char* ping = "/sensors/ping";

HttpClient client(wifi, serverAddress, serverPort);



/*****************************************
*   Define sensor pins for Digital and Analog Here
*****************************************/

// Defined DHT pins
#define DHTPIN1 2
#define DHTPIN2 1
#define DHTTYPE DHT11
DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

//Defined Water Temp Pins
MicroDS18B20<3> sensor;

//Defined Buzzer Pins
#define BUZZER_PIN 9

// Defined Relay pins
#define HEATER_RELAY_PIN 7

// Defined Ambient Temp Sensor
byte NTCPin = A0;
#define SERIESRESISTOR 10000
#define NOMINAL_RESISTANCE 10000
#define NOMINAL_TEMPERATURE 25
#define BCOEFFICIENT 3950

// Define Rotary Encoder pins
#define ROTARY_PIN_A 52   // Change to the actual pin
#define ROTARY_PIN_B 53   // Change to the actual pin
#define ROTARY_BUTTON 51  // Change to the actual pin

// Define the initial target temperature
#define INITIAL_TEMP 20

/*****************************************
*   GLOBAL VARIABLES
*****************************************/
//ID Variables
String device_id;

//Temperature Variables
float temperature1;
float humidity1;
float ambientTemp;
float waterTemp;

// Defined Relay Temp threshold
float targetTemperature = INITIAL_TEMP;

// Define the current page variable and the number of pages
int currentPage = 0;
int lastPage = 0;
int numPages = 5;  // DHT Temp, Relay, Ambient Temp, Water Flow, Water Temp
volatile bool pageChangeDisabled = false;

// Define the variable to store the switch state
bool switchState = false;
bool lastSwitchState = LOW;

//Encoder prositions
volatile int encoderPos = 0;
volatile int lastEncoderPos = 0;

//Track time for Sensor updates
unsigned long previousMillis = 0;
const long interval = 30000;  //1000 per second

//Track time for sending Sensor data to server
unsigned long sendDataPreviousMillis = 0;
const long sendDataInterval = 30000;  //1000 per second

//Track time for sending pings to the server (Used to track device status)
unsigned long sendPingPreviousMillis = 0;
const long sendPingInterval = 60000;

//Debug Messages
char heaterStatus;

//Storage Variables for Sensor Data
struct sensorData {
  String name;
  String sensorName;
  unsigned long timestamp;
  String sensorType;
  String sensorLocation;
  String dataType;
  float data;
};

// pH Sensor Module + pH Electrode Probe BNC
int analogPin = A1;  // Analog input pin for pH sensor
float phValue;       // Variable to store pH value

/*****************************************
*   SETUP FUNCTION
*****************************************/
void setup() {
  Serial.begin(9600);

  //Get ID's
  device_id = "GG-001";

  pinMode(HEATER_RELAY_PIN, OUTPUT);    // Set pinMode for the Heater Relay Pin
  digitalWrite(HEATER_RELAY_PIN, LOW);  // Initially, turn the relay off
  pinMode(BUZZER_PIN, OUTPUT);

  //Initilaize DHT Sensors
  dht1.begin();
  dht2.begin();

  // Initialize the rotary encoder pins
  initEncoder();

  //Start LCD Screen --> Show boot Screen
  useLCD();

  // playBootSound(BUZZER_PIN);  //Play Boot Sound
  bootScreen();   //Display Boot Screen
  connectWiFi();  // Establish Wifi Connection

  // Initialize NTP Client
  timeClient.begin();


  //Test Connection with API
  makeGetRequest(serverTest);
}


/*****************************************
*   MAIN PROGRAM LOOP
*****************************************/

void loop() {

  //Timer for Sensor Readings
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    Serial.println("Reading Temerature");

    readDHT();
    readAmbientTemp();
    readWaterTemps();
    readPH();

    setRelay1(HEATER_RELAY_PIN, temperature1, targetTemperature);

    debugInfo();


    lcd.clear();
  }

  //Timer for sending sensor data to the server
  unsigned long sendDataCurrentMillis = millis();
  if (sendDataCurrentMillis - sendDataPreviousMillis >= sendDataInterval) {
    sendDataPreviousMillis = sendDataCurrentMillis;
    postSensorData(serverRoute);
  }

  //Timer for Sending Pings to the server
  unsigned long sendPingCurrentMillis = millis();
  if (sendPingCurrentMillis - sendPingPreviousMillis >= sendPingInterval) {
    sendPingPreviousMillis = sendPingCurrentMillis;

    //Send the Ping To the Server
    makeGetRequest(ping);
  }

  // Check if the button is pressed
  switchState = digitalRead(ROTARY_BUTTON);  // Read the switch state
  if (switchState == LOW && lastSwitchState == HIGH) {

    // If the Button is pressed on set page, Toggle alternate Page (Used for settings, etc)
    switch (currentPage) {
      case 1:  // Heater Page
        // Toggle the mode when the button is pressed
        pageChangeDisabled = !pageChangeDisabled;  // Switch between true and false
        break;
    }

    // Print a message to indicate the change
    Serial.print("Button pressed, pageChangeDisabled is now ");
    Serial.println(pageChangeDisabled ? "ON" : "OFF");

    setRelay1(HEATER_RELAY_PIN, temperature1, targetTemperature);

    lcd.clear();
  }

  // Remember the current switch state for the next loop iteration
  lastSwitchState = switchState;


  // Displays the LCD Pages
  if (pageChangeDisabled == false) {

    getEncoderPosition();

    // Display the appropriate page data based on the current page
    switch (currentPage) {
      case 0:
        displayDHTData(temperature1, humidity1, currentPage, lastPage);
        lastPage = currentPage;
        break;
      case 1:
        displayHeaterStatus(HEATER_RELAY_PIN, temperature1, targetTemperature, currentPage, lastPage);
        lastPage = currentPage;
        break;
      case 2:
        displayAmbientTemp(ambientTemp, currentPage, lastPage);
        lastPage = currentPage;
        break;
      case 3:
        displayWaterFlow(currentPage, lastPage);
        lastPage = currentPage;
        break;
      case 4:
        displayWaterTemp(waterTemp, currentPage, lastPage);
        lastPage = currentPage;
        break;
    }
  } else {

    // Display the appropriate Button Press Page Data
    switch (currentPage) {
      case 0:
        break;
      case 1:
        displayTempChange(targetTemperature, currentPage, lastPage);
        break;
      case 2:
        break;
      case 3:
        break;
      case 4:
        break;
    }
  }

  delay(500);
}

/*************************************************
*       Debug and Com Message Functions Below
************************************************/


void debugInfo() {
  Serial.print("Ambient Temperature: ");
  Serial.println(ambientTemp);
  Serial.print("DHT Temp Sensor 1: ");
  Serial.println(temperature1);
  Serial.print("DHT Humidity Sensor 1: ");
  Serial.println(humidity1);
  Serial.print("Water Temperature Sensor 1: ");
  Serial.println(waterTemp);
  Serial.print("pH: ");
  Serial.println(phValue, 2);  // Print the pH value with two decimal places

  int relayStatus = digitalRead(HEATER_RELAY_PIN);

  if (relayStatus == LOW) {
    Serial.println("Heater is ON");
  } else {
    Serial.println("Heater is OFF");
  }
}


/*************************************************
*       Sensor Reading Functions Below
************************************************/

const int sensorArray_Size = 100;

//Read the Temperature and Humidity
sensorData tempData[sensorArray_Size];
sensorData humidityData[sensorArray_Size];
// if errors with temp might need to change from INT
int currentIndexForDHT = 0;

void readDHT() {

  if (isnan(dht1.readTemperature())) {
    temperature1 = 0;
    humidity1 = 0;
    return;
  }

  temperature1 = dht1.readTemperature();
  humidity1 = dht1.readHumidity();

  if (currentIndexForDHT < sensorArray_Size) {

    //Sensor Information
    tempData[currentIndexForDHT].name = "Temperature Sensor";
    tempData[currentIndexForDHT].sensorName = "Sensor 1";
    tempData[currentIndexForDHT].sensorType = "DHT";
    tempData[currentIndexForDHT].sensorLocation = "Greenhouse 1";
    tempData[currentIndexForDHT].dataType = "Temperature";

    //Sensor Data
    tempData[currentIndexForDHT].data = temperature1;
    tempData[currentIndexForDHT].timestamp = getCurrentTime();

    //Sensor Information
    humidityData[currentIndexForDHT].name = "Humidity Sensor";
    humidityData[currentIndexForDHT].sensorName = "Sensor 1";
    humidityData[currentIndexForDHT].sensorType = "DHT";
    humidityData[currentIndexForDHT].sensorLocation = "Greenhouse 1";
    humidityData[currentIndexForDHT].dataType = "Humidity";

    //Sensor Data
    humidityData[currentIndexForDHT].data = humidity1;
    humidityData[currentIndexForDHT].timestamp = getCurrentTime();


    currentIndexForDHT++;
  } else {
    resetSensorArray();
  }
}

//Read the Device Temperature
sensorData deviceTempData[sensorArray_Size];
int currentIndexForTemp = 0;

void readAmbientTemp() {

  if (!analogRead(NTCPin)) {
    ambientTemp = 0;
    return;
  }

  float ADCvalue;
  float Resistance;

  ADCvalue = analogRead(NTCPin);
  Resistance = (1023.0 / ADCvalue) - 1.0;
  Resistance = SERIESRESISTOR / Resistance;
  ambientTemp = Resistance / NOMINAL_RESISTANCE;        // (R/Ro)
  ambientTemp = log(ambientTemp);                       // ln(R/Ro)
  ambientTemp /= BCOEFFICIENT;                          // 1/B * ln(R/Ro)
  ambientTemp += 1.0 / (NOMINAL_TEMPERATURE + 273.15);  // + (1/To)
  ambientTemp = 1.0 / ambientTemp;                      // Invert
  ambientTemp -= 273.15;                                // convert to C

  if (currentIndexForTemp < sensorArray_Size) {

    //Sensor information
    deviceTempData[currentIndexForTemp].name = "Device Temperature";
    deviceTempData[currentIndexForTemp].sensorName = "Sensor 1";
    deviceTempData[currentIndexForTemp].sensorType = "Internal";
    deviceTempData[currentIndexForTemp].sensorLocation = "Default";
    deviceTempData[currentIndexForTemp].dataType = "Temperature";

    //Sensor Data
    deviceTempData[currentIndexForTemp].data = ambientTemp;
    deviceTempData[currentIndexForTemp].timestamp = getCurrentTime();
    currentIndexForTemp++;
  } else {
    resetSensorArray();
  }
}

//Read the water Temperature
sensorData waterTempData[sensorArray_Size];
int currentIndexForWaterTemp = 0;

void readWaterTemps() {
  sensor.requestTemp();

  if (!sensor.readTemp()) {
    waterTemp = 0;
    return;
  }

  //Read the Sensor
  float data = sensor.getTemp();
  waterTemp = data;

  if (currentIndexForWaterTemp < sensorArray_Size) {
    //Sensor Information
    waterTempData[currentIndexForWaterTemp].name = "Water Temperature";
    waterTempData[currentIndexForWaterTemp].sensorName = "Sensor 1";
    waterTempData[currentIndexForWaterTemp].sensorType = "ds18b20";
    waterTempData[currentIndexForWaterTemp].sensorLocation = "Greenhouse 1";
    waterTempData[currentIndexForWaterTemp].dataType = "Temperature";

    waterTempData[currentIndexForWaterTemp].data = waterTemp;
    waterTempData[currentIndexForWaterTemp].timestamp = getCurrentTime();
    currentIndexForWaterTemp++;
  } else {
    resetSensorArray();
  }
}

//Ready the PH Sensor
sensorData phData[sensorArray_Size];
int currentIndexForPH = 0;

void readPH() {

  //Default to PH 0 If sensor is not connected - Values of 0 are excluded from JSON Document
  if (!analogRead(analogPin)) {
    phValue = 0;
    return;
  }

  //Read The Sensor
  phValue = analogRead(analogPin);         // Read the analog value from sensor
  phValue = 3.5 * (phValue * 5.0 / 1024);  // Convert the analog value to pH value

    if (currentIndexForPH < sensorArray_Size) {
    //Sensor Information
    phData[currentIndexForPH].name = "PH";
    phData[currentIndexForPH].sensorName = "Sensor 1";
    phData[currentIndexForPH].sensorType = "BNC PH Probe";
    phData[currentIndexForPH].sensorLocation = "Greenhouse 1";
    phData[currentIndexForPH].dataType = "PH";

    phData[currentIndexForPH].data = phValue;
    phData[currentIndexForPH].timestamp = getCurrentTime();
    currentIndexForPH++;
  } else {
    resetSensorArray();
  }
}


// Reset the Sensor Array values to 0
void resetSensorArray() {

  currentIndexForDHT = 0;
  currentIndexForTemp = 0;
  currentIndexForWaterTemp = 0;
  currentIndexForPH = 0;

  for (int i = 0; i < sensorArray_Size; i++) {
    tempData[i].data = 0;
    humidityData[i].data = 0;
    deviceTempData[i].data = 0;
    waterTempData[i].data = 0;
    phData[i].data = 0;
  }
}

/*****************************************
*   Rotary Encoder functions
      - Initializes the Encoder
      - Stores Encoder Positions
      - Handles the Encoder Rotation and Button Press
*****************************************/

//Function to set the rotary encoder pins and interupts
void initEncoder() {
  // Initialize the rotary encoder pins
  pinMode(ROTARY_PIN_A, INPUT_PULLUP);
  pinMode(ROTARY_PIN_B, INPUT_PULLUP);
  pinMode(ROTARY_BUTTON, INPUT_PULLUP);

  //Attach Interrupt to the Left and Right Turning of the Encoder Nob
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), handleEncoder, CHANGE);  //left
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), handleEncoder, CHANGE);  //right
}

//Determine the current Position of the Encoder to track Pages
void getEncoderPosition() {

  // Handle rotary encoder rotation for Page Changes
  if (encoderPos != lastEncoderPos) {
    if (encoderPos > lastEncoderPos) {
      currentPage = (currentPage + 1) % numPages;
    } else {
      currentPage = (currentPage - 1 + numPages) % numPages;
    }
    lastEncoderPos = encoderPos;
  }
}


// Function to Controll the Rotary Controller Turns
void handleEncoder() {
  static unsigned int lastEncoded = 0;
  static unsigned int newEncoded = 0;

  int MSB = digitalRead(ROTARY_PIN_A);
  int LSB = digitalRead(ROTARY_PIN_B);

  // Handles changing the target temperature on the Heater Screen
  if (pageChangeDisabled == true) {

    newEncoded = (MSB << 1) | LSB;
    int sum = (lastEncoded << 2) | newEncoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
      targetTemperature++;  // Increase the target temperature by one degree
    } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
      targetTemperature--;  // Decrease the target temperature by one degree
    }

    lastEncoded = newEncoded;
  }

  //When in standard page view handle the changing of pages
  if (pageChangeDisabled == false) {

    newEncoded = (MSB << 1) | LSB;
    int sum = (lastEncoded << 2) | newEncoded;
    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
      encoderPos++;
    } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
      encoderPos--;
    }

    lastEncoded = newEncoded;
  }
}


/*****************************************
*   wifi connection functions
      - Handles Connection to WiFi Network
      - Stores WiFi Information
      - Stores Connected MAC Address Information
*****************************************/
void connectWiFi() {

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    lcd.clear();
    lcd.setCursor(0, 2);
    lcd.print("Wifi");
    lcd.setCursor(0, 3);
    lcd.print("Connection Failed");
    delay(2000);
    while (true)
      ;
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);

    lcd.setCursor(0, 2);
    lcd.print("Connecting to");
    lcd.setCursor(0, 3);
    lcd.print("Network");

    status = WiFi.begin(ssid, pass);
    delay(2000);
  }

  lcd.clear();
  lcd.setCursor(0, 2);
  lcd.print("Connected to: ");
  lcd.setCursor(0, 3);
  lcd.print(String(ssid));
  delay(2000);

  Serial.println("You're connected to the network");
  printWifiStatus();
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}


/*****************************************
*   Server Request Functions
      - For Communication with the Node.JS API Server
        - Handles HTTP Requests (GET, POST)
*****************************************/

void makeGetRequest(const char* serverRoute) {
  client.stop();

  String queryString = "?deviceID=" + device_id;

  Serial.println("Attempting to Connect to API Server");

  //Send a Get Request to the Server
  client.get(serverRoute + queryString);

  //Check if the Connection was Successfull
  if (!client.connected()) {
    Serial.println("Failed to Connect to API Server");
    return;
  }

  // Wait for a response with a timeout
  unsigned long startTime = millis();
  while (!client.available()) {
    if (millis() - startTime > 5000) {  // 5 second timeout
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



String convertToJSON() {
  StaticJsonDocument<1000> doc;  // Create a static JSON document.

  JsonArray Data = doc.createNestedArray("Data");

  for (int i = 0; i < sensorArray_Size; i++) {

    //Check if the iteration has no data
    if (deviceTempData[i].data != 0 || tempData[i].data != 0 || humidityData[i].data != 0 || waterTempData[i].data != 0 || phData[i].data != 0) {

      //If there not all 0 then create an object to store this iterations data
      JsonObject sensorDataObject = Data.createNestedObject();

      JsonObject DeviceInfo = sensorDataObject.createNestedObject("Device");
      DeviceInfo["DeviceID"] = device_id;

      JsonArray SensorReadings = sensorDataObject.createNestedArray("SensorReadings");

      addSensorReading(SensorReadings, deviceTempData[i]);
      addSensorReading(SensorReadings, tempData[i]);
      addSensorReading(SensorReadings, humidityData[i]);
      addSensorReading(SensorReadings, waterTempData[i]);
      addSensorReading(SensorReadings, phData[i]);
    }
  }
  // Convert the JSON document to a string
  String postData;
  serializeJson(doc, postData);

  return postData;
}

void addSensorReading(JsonArray& SensorReadings, sensorData sensor) {
  //If there is Data
  if (sensor.data != 0) {
    JsonObject reading = SensorReadings.createNestedObject();

    reading["Name"] = sensor.name;
    reading["Value"] = sensor.data;
    reading["Time"] = sensor.timestamp;

    if (!sensor.sensorName.isEmpty()) {
      reading["Sensor"] = sensor.sensorName;
    }
    if (!sensor.sensorType.isEmpty()) {
      reading["Type"] = sensor.sensorType;
    }
    if (!sensor.dataType.isEmpty()) {
      reading["Field"] = sensor.dataType;
    }
    if (!sensor.sensorLocation.isEmpty()) {
      reading["Location"] = sensor.sensorLocation;
    }
  }
}


void postSensorData(const char* serverRoute) {

  Serial.println("making POST request");

  String contentType = "application/json";
  String postData = convertToJSON();

  client.beginRequest();
  client.post(serverRoute);

  //Check if the Connection was Successfull
  if (!client.connected()) {
    client.stop();
    Serial.println("Failed to send Data");
    return;
  }

  client.sendHeader("Content-Type", contentType);
  client.sendHeader("Content-Length", postData.length());
  client.beginBody();
  client.print(postData);
  client.endRequest();

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  if (statusCode > 0) {
    Serial.print("HTTP Response Status Code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);

    resetSensorArray();

  } else {
    Serial.println("HTTP Request failed");
  }
}