/*****************************************
 *  Imported Libraries and files
 *****************************************/
// Arduino Libraries
#include <Arduino.h>
#include "definitions.h"

// Connections
#include <DHT.h>
#include <microDS18B20.h>

// Wire Inputs
#include <Wire.h>

// Sensitive Data
#include "secrets.h"

// import Directory Files
#include "relayControl.h"
#include "getTime.h"
#include "wifiControl.h"
#include "serverRequest.h"
#include "jsonFunctions.h"

/*****************************************
 *   Communications for WIFI and Server
 *****************************************/
// Connection Info
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

const char *serverAddress = SECRET_API_SERVER;
const int serverPort = SECRET_PORT;
const char *serverRoute = "/sensors/sendData";
const char *serverRouteGet = "/sensors/retrieve";
const char *serverTest = "/sensors/testconnection";
const char *ping = "/sensors/ping";

// ssl wifi client
WiFiClient wifi;

HttpClient client(wifi, serverAddress, serverPort);

/*****************************************
 *   Define sensor pins for Digital and Analog Here
 *****************************************/

DHT dht1(DHTPIN1, DHTTYPE);

// Defined Water Temp Pins
MicroDS18B20<3> sensor;

/*****************************************
 *   GLOBAL VARIABLES
 *****************************************/
// ID Variables
String device_id;

// Temperature Variables
float temperature1;
float humidity1;
float ambientTemp;
float waterTemp;

// Defined Relay Temp threshold
float targetTemperature = INITIAL_TEMP;

// Track time for Sensor updates
unsigned long previousMillis = 0;
const long interval = 30000; // 1000 per second

// Track time for sending Sensor data to server
unsigned long sendDataPreviousMillis = 0;
const long sendDataInterval = 30000; // 1000 per second

// Track time for sending pings to the server (Used to track device status)
unsigned long sendPingPreviousMillis = 0;
const long sendPingInterval = 60000;

// Debug Messages
char heaterStatus;

// Storage Variables for Sensor Data
struct sensorData
{
  String name;
  String sensorName;
  unsigned long timestamp;
  String sensorType;
  String sensorLocation;
  String dataType;
  float data;
};

// pH Sensor Module + pH Electrode Probe BNC
float phValue; // Variable to store pH value

int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;

// Initialize the Relays
RelayControl relay1(HEATER_RELAY_PIN_1, 5.0);
RelayControl relay2(HEATER_RELAY_PIN_2, 5.0);
RelayControl relay3(HEATER_RELAY_PIN_3, 5.0);
RelayControl relay4(HEATER_RELAY_PIN_4, 5.0);
RelayControl relays[] = {relay1, relay2, relay3, relay4};

/*****************************************
 *   SETUP FUNCTION
 *****************************************/
void setup()
{
  Serial.begin(115200);

  // set/get ID's
  device_id = "GG-001";

  // Initialize The Relays
  for (RelayControl &relay : relays)
  {
    relay.initialize();
  }

  // Initilaize DHT Sensors
  //  dht1.begin();

  // Initialize the TDS Pin
  //  pinMode(TdsSensorPin, INPUT);

  // Wifi Connection

  // Test Connection with API
  //  makeGetRequest(serverTest);
}

/*****************************************
 *   MAIN PROGRAM LOOP
 *****************************************/

void loop()
{
  // Check & Set relay status for each relay
  for (RelayControl &relay : relays)
  {
    relay.checkRelay(temperature1, targetTemperature);
  }

  // Timer for Sensor Readings
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    Serial.println("Reading Temerature");

    readDHT();
    readAmbientTemp();
    readWaterTemps();
    readPH();
    readTDS();
  }

  // Timer for sending sensor data to the server
  unsigned long sendDataCurrentMillis = millis();
  if (sendDataCurrentMillis - sendDataPreviousMillis >= sendDataInterval)
  {
    sendDataPreviousMillis = sendDataCurrentMillis;
    // postSensorData(serverRoute);
  }

  // Timer for Sending Pings to the server
  unsigned long sendPingCurrentMillis = millis();
  if (sendPingCurrentMillis - sendPingPreviousMillis >= sendPingInterval)
  {
    sendPingPreviousMillis = sendPingCurrentMillis;

    // Send the Ping To the Server
    //  makeGetRequest(ping);
  }
}

/*************************************************
 *       Debug and Com Message Functions Below
 ************************************************/

void debugInfo()
{
  Serial.print("Ambient Temperature: ");
  Serial.println(ambientTemp);
  Serial.print("DHT Temp Sensor 1: ");
  Serial.println(temperature1);
  Serial.print("DHT Humidity Sensor 1: ");
  Serial.println(humidity1);
  Serial.print("Water Temperature Sensor 1: ");
  Serial.println(waterTemp);
  Serial.print("pH: ");
  Serial.println(phValue, 2); // Print the pH value with two decimal places
  Serial.print("TDS Value:");
  Serial.print(tdsValue, 0);
  Serial.println("ppm");

  int relayStatus = digitalRead(HEATER_RELAY_PIN);

  if (relayStatus == LOW)
  {
    Serial.println("Heater is ON");
  }
  else
  {
    Serial.println("Heater is OFF");
  }
}

/*************************************************
 *       Sensor Reading Functions Below
 ************************************************/

// Read the Temperature and Humidity
sensorData tempData[sensorArray_Size];
sensorData humidityData[sensorArray_Size];
int currentIndexForDHT = 0;

void readDHT()
{

  if (isnan(dht1.readTemperature()))
  {
    temperature1 = 0;
    humidity1 = 0;
    return;
  }

  temperature1 = dht1.readTemperature();
  humidity1 = dht1.readHumidity();

  if (currentIndexForDHT < sensorArray_Size)
  {

    // Sensor Information
    tempData[currentIndexForDHT].name = "Temperature Sensor";
    tempData[currentIndexForDHT].sensorName = "Sensor 1";
    tempData[currentIndexForDHT].sensorType = "DHT";
    tempData[currentIndexForDHT].sensorLocation = "Greenhouse 1";
    tempData[currentIndexForDHT].dataType = "Temperature";

    // Sensor Data
    tempData[currentIndexForDHT].data = temperature1;
    tempData[currentIndexForDHT].timestamp = getCurrentTime();

    // Sensor Information
    humidityData[currentIndexForDHT].name = "Humidity Sensor";
    humidityData[currentIndexForDHT].sensorName = "Sensor 1";
    humidityData[currentIndexForDHT].sensorType = "DHT";
    humidityData[currentIndexForDHT].sensorLocation = "Greenhouse 1";
    humidityData[currentIndexForDHT].dataType = "Humidity";

    // Sensor Data
    humidityData[currentIndexForDHT].data = humidity1;
    humidityData[currentIndexForDHT].timestamp = getCurrentTime();

    currentIndexForDHT++;
  }
  else
  {
    resetSensorArray();
  }
}

// Read the Device Temperature
sensorData deviceTempData[sensorArray_Size];
int currentIndexForTemp = 0;

void readAmbientTemp()
{

  if (!analogRead(NTCPin))
  {
    ambientTemp = 0;
    return;
  }

  // Read the Analog Signal and convert it to Readable Temperate
  float ADCvalue;
  float Resistance;

  ADCvalue = analogRead(NTCPin);
  Resistance = (1023.0 / ADCvalue) - 1.0;
  Resistance = SERIESRESISTOR / Resistance;
  ambientTemp = Resistance / NOMINAL_RESISTANCE;       // (R/Ro)
  ambientTemp = log(ambientTemp);                      // ln(R/Ro)
  ambientTemp /= BCOEFFICIENT;                         // 1/B * ln(R/Ro)
  ambientTemp += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
  ambientTemp = 1.0 / ambientTemp;                     // Invert
  ambientTemp -= 273.15;                               // convert to C

  if (currentIndexForTemp < sensorArray_Size)
  {

    // Sensor information
    deviceTempData[currentIndexForTemp].name = "Device Temperature";
    deviceTempData[currentIndexForTemp].sensorName = "Sensor 1";
    deviceTempData[currentIndexForTemp].sensorType = "Internal";
    deviceTempData[currentIndexForTemp].sensorLocation = "Default";
    deviceTempData[currentIndexForTemp].dataType = "Temperature";

    // Sensor Data
    deviceTempData[currentIndexForTemp].data = ambientTemp;
    deviceTempData[currentIndexForTemp].timestamp = getCurrentTime();
    currentIndexForTemp++;
  }
  else
  {
    resetSensorArray();
  }
}

// Read the water Temperature
sensorData waterTempData[sensorArray_Size];
int currentIndexForWaterTemp = 0;

void readWaterTemps()
{
  sensor.requestTemp();

  if (!sensor.readTemp())
  {
    waterTemp = 0;
    return;
  }

  // Read the Sensor
  float data = sensor.getTemp();
  waterTemp = data;

  if (currentIndexForWaterTemp < sensorArray_Size)
  {
    // Sensor Information
    waterTempData[currentIndexForWaterTemp].name = "Water Temperature";
    waterTempData[currentIndexForWaterTemp].sensorName = "Sensor 1";
    waterTempData[currentIndexForWaterTemp].sensorType = "ds18b20";
    waterTempData[currentIndexForWaterTemp].sensorLocation = "Greenhouse 1";
    waterTempData[currentIndexForWaterTemp].dataType = "Temperature";

    waterTempData[currentIndexForWaterTemp].data = waterTemp;
    waterTempData[currentIndexForWaterTemp].timestamp = getCurrentTime();
    currentIndexForWaterTemp++;
  }
  else
  {
    resetSensorArray();
  }
}

// Read the PH Sensor
sensorData phData[sensorArray_Size];
int currentIndexForPH = 0;

void readPH()
{

  // Default to PH 0 If sensor is not connected - Values of 0 are excluded from JSON Document
  if (!analogRead(PHPIN))
  {
    phValue = 0;
    return;
  }

  // Read The Sensor
  phValue = analogRead(PHPIN);            // Read the analog value from sensor
  phValue = 3.5 * (phValue * 5.0 / 1024); // Convert the analog value to pH value

  // Temporry disable for PH Sensor Recordings
  phValue = 0;

  if (currentIndexForPH < sensorArray_Size)
  {
    // Sensor Information
    phData[currentIndexForPH].name = "PH";
    phData[currentIndexForPH].sensorName = "PH Sensor 1";
    phData[currentIndexForPH].sensorType = "BNC PH Probe";
    phData[currentIndexForPH].sensorLocation = "Greenhouse 1";
    phData[currentIndexForPH].dataType = "PH";

    phData[currentIndexForPH].data = phValue;
    phData[currentIndexForPH].timestamp = getCurrentTime();
    currentIndexForPH++;
  }
  else
  {
    resetSensorArray();
  }
}

// Read the TDS
sensorData tdsData[sensorArray_Size];
int currentIndexForTDS = 0;

void readTDS()
{

  // If there is no reading Do nothing (If no sensor, or when initializing ignore data)
  if (tdsValue == 0)
  {
    return;
  }

  if (currentIndexForTDS < sensorArray_Size)
  {

    // Sensor Information
    tdsData[currentIndexForTDS].name = "TDS";
    tdsData[currentIndexForTDS].sensorName = "TDS Sensor 1";
    tdsData[currentIndexForTDS].sensorType = "TDS";
    tdsData[currentIndexForTDS].sensorLocation = "Greenhouse 1";
    tdsData[currentIndexForTDS].dataType = "PPM";

    // Sensor Data
    tdsData[currentIndexForTDS].data = tdsValue;
    tdsData[currentIndexForTDS].timestamp = getCurrentTime();
    currentIndexForTDS++;
  }
  else
  {
    resetSensorArray();
  }
}

// Reset the Sensor Array values to 0
void resetSensorArray()
{

  currentIndexForDHT = 0;
  currentIndexForTemp = 0;
  currentIndexForWaterTemp = 0;
  currentIndexForPH = 0;
  currentIndexForTDS = 0;

  for (int i = 0; i < sensorArray_Size; i++)
  {
    tempData[i].data = 0;
    humidityData[i].data = 0;
    deviceTempData[i].data = 0;
    waterTempData[i].data = 0;
    phData[i].data = 0;
    tdsData[i].data = 0;
  }
}

/*****************************************
 *   Functions to Store the TDS Readings
 *****************************************/
float getTDSReading()
{

  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) // every 40 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); // read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }

  // every 800 ms read the tds from buffer, convert to readable value
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U)
  {

    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;                                                                                                  // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);                                                                                                               // temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = averageVoltage / compensationCoefficient;                                                                                                            // temperature compensation
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; // convert voltage value to tds value
  }
}

int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}