#include <Arduino.h>
#include "definitions.h"


int lightSensorValue = 0;

// Function prototype
int readLightSensor();

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  // put your main code here, to run repeatedly:
  lightSensorValue = readLightSensor();

  // Raw value from light sensor
  Serial.print("Raw Light level: ");
  Serial.println(lightSensorValue);

  // map the value from 0, 1023 to 0, 100
  int lightAsPercentage = map(lightSensorValue, 0, 1023, 0, 100);
  Serial.print("Light level: ");
  Serial.print(lightAsPercentage); // print the light value in Serial Monitor
  Serial.println("%");

  delay(3000);
}

// put function definitions here:
int readLightSensor()
{
  int sensorValue = analogRead(LightSensor_Pin);
  return sensorValue;
}