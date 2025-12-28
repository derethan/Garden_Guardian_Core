/* NTC Thermistor (Adafruit-style sample) + Relay control with hysteresis
   Hardware:
   - NTC thermistor divided with SERIESRESISTOR (10k) to A0
   - Relay control pin: D3 (assumed active HIGH; change if needed)
*/

#include <Arduino.h>
#include <math.h>

// Pins
const uint8_t NTCPin = A0;
const uint8_t relayPin = 3;
const int ledPin = 13; // Built-in LED

// Thermistor constants (from your sample)
#define SERIESRESISTOR 10000.0     // the fixed series resistor value (Ohms)
#define NOMINAL_RESISTANCE 10000.0 // resistance at nominal temperature (Ohms)
#define NOMINAL_TEMPERATURE 25.0   // nominal temperature (°C)
#define BCOEFFICIENT 3950.0        // B coefficient of the thermistor

// Sampling and timing
const unsigned long SAMPLE_INTERVAL_MS = 1000UL; // read every 1 second

// Hysteresis thresholds (°C)
// Heater turns ON when temp <= ON_THRESHOLD
// Heater turns OFF when temp >= OFF_THRESHOLD
const float ON_THRESHOLD = 20.0;  // lower threshold — turn heater ON at or below this
const float OFF_THRESHOLD = 25.0; // upper threshold — turn heater OFF at or above this

// Relay polarity (adjust if your relay module is active-LOW)
const uint8_t RELAY_ON = HIGH;
const uint8_t RELAY_OFF = LOW;

bool heaterOn = false;
unsigned long lastSample = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);

  // start with heater off
  digitalWrite(relayPin, RELAY_OFF);
  heaterOn = false;

  Serial.println(F("NTC Thermistor + Relay (hysteresis)"));
  Serial.print(F("ON_THRESHOLD = "));
  Serial.print(ON_THRESHOLD);
  Serial.println(F(" C"));
  Serial.print(F("OFF_THRESHOLD = "));
  Serial.print(OFF_THRESHOLD);
  Serial.println(F(" C"));
  Serial.println();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Start LED OFF
}

void loop()
{
  unsigned long now = millis();
  if (now - lastSample < SAMPLE_INTERVAL_MS)
    return;
  lastSample = now;

  // 1) Read ADC
  float adc = analogRead(NTCPin);
  Serial.print(F("ADC: "));
  Serial.print(adc);

  // protect against divide-by-zero (rare but possible if ADC returns 0)
  if (adc <= 0.0)
  {
    Serial.println(F("  <-- ADC reading 0, skipping"));
    return;
  }

  // 2) Convert ADC to resistance
  // Your formula: Resistance = SERIESRESISTOR / ((1023/ADCvalue) - 1)
  float resistance = (1023.0 / adc) - 1.0;
  resistance = SERIESRESISTOR / resistance;
  Serial.print(F("  R = "));
  Serial.print(resistance);
  Serial.print(F(" Ohm"));

  // 3) Steinhart–Hart (B-parameter) conversion to Celsius
  float steinhart;
  steinhart = resistance / NOMINAL_RESISTANCE;       // (R/Ro)
  steinhart = log(steinhart);                        // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                         // 1/B * ln(R/Ro)
  steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                       // Invert
  float temperatureC = steinhart - 273.15;           // convert to C

  Serial.print(F("  Temp = "));
  Serial.print(temperatureC);
  Serial.println(F(" C"));

  // 4) Hysteresis control logic
  if (!heaterOn && (temperatureC <= ON_THRESHOLD))
  {
    heaterOn = true;
    digitalWrite(relayPin, RELAY_ON);
    digitalWrite(ledPin, HIGH); // LED ON
    Serial.println(F("Heater -> ON"));
  }
  else if (heaterOn && (temperatureC >= OFF_THRESHOLD))
  {
    heaterOn = false;
    digitalWrite(relayPin, RELAY_OFF);
    digitalWrite(ledPin, LOW); // LED OFF
    Serial.println(F("Heater -> OFF"));
  }
  else
  {
    // no state change
    Serial.print(F("Heater state unchanged: "));
    Serial.println(heaterOn ? F("ON") : F("OFF"));
  }

  Serial.println();
}
