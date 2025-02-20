// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_DHT_Particle.h"

// include arduino
#include "Arduino.h"

// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#define DHTPIN D2 // what pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11 // DHT 11

// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

DHT dht(DHTPIN, DHTTYPE);
int loopCount;
unsigned long previousMillis = 0;
unsigned long interval = 60000;

void setup()
{
	Serial.begin(9600);
	Serial.println("DHTxx test!");
	Particle.publish("state", "Nursery Monitor Started");

	dht.begin();
	loopCount = 0;
	delay(2000);
}

void loop()
{

	// basic millis  timer
	unsigned long currentMillis = millis();

	if (currentMillis - previousMillis >= interval)
	{
		// Reading temperature or humidity takes about 250 milliseconds!
		float h = dht.getHumidity();
		// Read temperature as Celsius
		float t = dht.getTempCelcius();
		// Read temperature as Farenheit
		float f = dht.getTempFarenheit();

		// Check if any reads failed and exit early (to try again).
		if (isnan(h) || isnan(t) || isnan(f))
		{
			Serial.println("Failed to read from DHT sensor!");
			return;
		}

		// Compute heat index
		// Must send in temp in Fahrenheit!
		float hi = dht.getHeatIndex();
		float dp = dht.getDewPoint();
		float k = dht.getTempKelvin();

		Serial.print("Humid: ");
		Serial.print(h);
		Serial.print("% - ");
		Serial.print("Temp: ");
		Serial.print(t);
		Serial.print("*C ");
		Serial.print("DewP: ");
		Serial.print(dp);
		Serial.print("*C - ");
		Serial.println(Time.timeStr());
		String timeStamp = Time.timeStr();
		Particle.publish("readings", String::format("{\"Time\": \"%s\", \"Hum(%)\": %4.2f, \"Temp(°C)\": %4.2f, \"DP(°C)\": %4.2f, \"HI(°C)\": %4.2f}", timeStamp.c_str(), h, t, dp, hi));
		previousMillis = currentMillis;
	}
}

// loopCount++;
// if(loopCount >= 6){
//   Particle.publish("state", "Going to sleep for 5 minutes");
//   delay(1000);
//   System.sleep(SLEEP_MODE_DEEP, 300);
// }