/*****************************************
*  Imported Libraries and files
*****************************************/
#include <SPI.h>
#include <RPC.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include "pitches.h"

//Connections
#include <DHT.h>
//LCD Display
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C address 0x27

//Wire Inputs
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>


//Sensitive Data
#include "arduino_secrets.h" 

//import Directory Files
#include "custom_char.h" 
#include "lcd_functions.h" 
#include "relay_control.h"
#include "buzzer_functions.h"


/*****************************************
*   Communications for WIFI and Server
*****************************************/
//Connection Info
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
WiFiClient client;

const char* serverAddress = SECRET_API_SERVER;
const int serverPort = SECRET_PORT;
const char* serverRoute = "/sensors/store";
const char* serverRouteGet = "/sensors/retrieve";
const char* serverTest = "/sensors/testconnection";



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
#define ONE_WIRE_BUS 3 // Change to the actual pin


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
#define ROTARY_PIN_A 52 // Change to the actual pin
#define ROTARY_PIN_B 53 // Change to the actual pin
#define ROTARY_BUTTON 51 // Change to the actual pin

// Define the initial target temperature
#define INITIAL_TEMP 20

/*****************************************
*   GLOBAL VARIABLES
*****************************************/
//Temperature Variables
float temperature1;
float humidity1;
float ambientTemp;

// Defined Relay Temp threshold
float targetTemperature = INITIAL_TEMP;

// Define the current page variable and the number of pages
int currentPage = 0;
int lastPage = 0;
int numPages = 4; // You have 4 pages - DHT, Relay, Ambient Temp, and Water Flow

volatile bool pageChangeDisabled = false;
bool lastSwitchState = LOW;

//Encoder prositions
volatile int encoderPos = 0;
volatile int lastEncoderPos = 0;

//Track time for Sensor updates
unsigned long previousMillis = 0;
const long interval = 30000; //1000 per second

//Debug Messages
char heaterStatus;


// Define the variable to store the switch state
bool switchState = false;
/*****************************************
*   SETUP FUNCTION
*****************************************/
void setup() {
  Serial.begin(9600);

  pinMode(HEATER_RELAY_PIN, OUTPUT); // Set pinMode for the Heater Relay Pin
  digitalWrite(HEATER_RELAY_PIN, LOW); // Initially, turn the relay off
  pinMode(BUZZER_PIN, OUTPUT);


  //Initilaize DHT Sensors
  dht1.begin();
  dht2.begin();
  readDHT ();

  // Initialize the rotary encoder pins
  initEncoder ();

  //Start LCD Screen --> Show boot Screen
  useLCD ();
  
  playBootSound(BUZZER_PIN);  //Play Boot Sound
  bootScreen ();  //Display Boot Screen
  connectWiFi (); // Establish Wifi Connection

  // //Test Connection with API
  makeGetRequest(serverTest);
}


/*****************************************
*   MAIN PROGRAM LOOP
*****************************************/

void loop() {

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      readDHT();
      readAmbientTemp ();

      setRelay1 (HEATER_RELAY_PIN, temperature1, targetTemperature);

      debugInfo();
      
      lcd.clear();

    }

  // Read the switch state
  switchState = digitalRead(ROTARY_BUTTON);

// Check if the button is pressed
  if (switchState == LOW && lastSwitchState == HIGH) {

  switch (currentPage) {
    case 1:
            // Toggle the mode when the button is pressed
            pageChangeDisabled = !pageChangeDisabled; // Switch between true and false
          break;
  }
  
    // Print a message to indicate the change
    Serial.print("Button pressed, pageChangeDisabled is now ");
    Serial.println(pageChangeDisabled ? "ON" : "OFF");

    setRelay1 (HEATER_RELAY_PIN, temperature1, targetTemperature);

    lcd.clear();

  }

  // Remember the current switch state for the next loop iteration
  lastSwitchState = switchState;

if (pageChangeDisabled == false) {

getEncoderPosition ();

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
      }
    }

delay (500);
}

/*************************************************
*       Debug and Com Message Functions Below
************************************************/

void debugInfo () {
    Serial.print("Ambient Temperature: ");
    Serial.println(ambientTemp);
    delay(1000);
    Serial.print("DHT Temp Sensor 1: ");
    Serial.println(temperature1);
    delay(1000);
    Serial.print("DHT Humidity Sensor 1: ");
    Serial.println(humidity1);

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

void readDHT (){
      temperature1 = dht1.readTemperature();
      humidity1 = dht1.readHumidity();
}

void readAmbientTemp () {
    float ADCvalue;
    float Resistance;

    ADCvalue = analogRead(NTCPin);
    Resistance = (1023.0 / ADCvalue) - 1.0;
    Resistance = SERIESRESISTOR / Resistance;
    ambientTemp = Resistance / NOMINAL_RESISTANCE; // (R/Ro)
    ambientTemp = log(ambientTemp); // ln(R/Ro)
    ambientTemp /= BCOEFFICIENT; // 1/B * ln(R/Ro)
    ambientTemp += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
    ambientTemp = 1.0 / ambientTemp; // Invert
    ambientTemp -= 273.15; // convert to C
}


/*****************************************
*   Rotary Encoder functions
      - Initializes the Encoder
      - Stores Encoder Positions
      - Handles the Encoder Rotation and Button Press
*****************************************/

//Function to set the rotary encoder pins and interupts
void initEncoder () {
    // Initialize the rotary encoder pins
  pinMode(ROTARY_PIN_A, INPUT_PULLUP);
  pinMode(ROTARY_PIN_B, INPUT_PULLUP);
  pinMode(ROTARY_BUTTON, INPUT_PULLUP);


  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), handleEncoder, CHANGE); //left
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), handleEncoder, CHANGE); //right

}

//Determine the current Position of the Encoder to track Pages
void  getEncoderPosition () {

      // Handle rotary encoder rotation
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

if (pageChangeDisabled == true) {

    newEncoded = (MSB << 1) | LSB;
    int sum = (lastEncoded << 2) | newEncoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
      targetTemperature++; // Increase the target temperature by one degree
    } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
      targetTemperature--; // Decrease the target temperature by one degree
    }

    lastEncoded = newEncoded;
  }


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
    lcd.clear ();
    lcd.setCursor(0, 2);
    lcd.print("Wifi");
    lcd.setCursor(0,3);
    lcd.print("Connection Failed");
    delay (2000);
    while (true);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);

    lcd.setCursor(0, 2);
    lcd.print("Connecting to");
    lcd.setCursor(0,3);
    lcd.print("Network");
    
    status = WiFi.begin(ssid, pass);
    delay (2000);
  }

  lcd.clear ();
  lcd.setCursor(0, 2);
  lcd.print("Connected to: ");
  lcd.setCursor(0,3);
  lcd.print(String(ssid));
  delay (2000);
  
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
  WiFiClient wifiClient;
  HttpClient client(wifiClient, serverAddress, serverPort);

  client.get(serverRoute);

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