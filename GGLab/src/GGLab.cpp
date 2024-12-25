/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
// #include "Adafruit_DHT.h"


// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);



// #define DHTPIN 2     // what pin we're connected to
// #define DHTTYPE DHT11		// DHT 11 


// DHT dht(DHTPIN, DHTTYPE);







// setup() runs once, when the device is first turned on
void setup() {
  // Put initialization like pinMode and begin functions here
  
  // setup for UART communication with Arduino
  Serial1.begin(9600);
  
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.

  // Example: Publish event to cloud every 10 seconds. Uncomment the next 3 lines to try it!
  // Log.info("Sending Hello World to the cloud!");
  // Particle.publish("Hello world!");
  // delay( 10 * 1000 ); // milliseconds and blocking - see docs for more info!


  // Example: Read data from Arduino and publish to cloud
  if (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    Log.info("Data from Arduino: %s", data.c_str());
    Particle.publish("data", data);
  }

  // Example: Read data from cloud and send to Arduino
  // if (Particle.connected()) {
  //   Particle.subscribe("data", myHandler, MY_DEVICES);
  // }

  // Example: Send data to Arduino
  // Serial1.println("Hello Arduino!");

  // Example: Read data from Arduino
  // if (Serial1.available()) {
  //   String data = Serial1.readStringUntil('\n');
  //   Log.info("Data from Arduino: %s", data.c_str());
  // }

  // Example: Read data from cloud
  // if (Particle.connected()) {
  //   Particle.subscribe("data", myHandler, MY_DEVICES);
  // }

  // Example: Send data to cloud
  // Particle.publish("data", "Hello cloud!");

  // Example: Send data to cloud every 10 seconds
  // static unsigned long lastTime = 0;
  // if (millis() - lastTime >= 10 * 1000) {
  //   Particle.publish("data", "Hello cloud!");
  //   lastTime = millis();
  // }
  

}
