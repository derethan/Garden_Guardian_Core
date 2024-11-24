/*************************************************
 *     Function to get time from NTP Serves
 *     Function to Convert Time to String (YY:MM:DD:HH:MM:SS)
 ************************************************/
#include <WiFi.h>
#include <time.h>
#include "getTime.h"

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;        // Adjust for your timezone (e.g., -18000 for EST)
const int daylightOffset_sec = 3600; // Daylight savings time offset

TimeRetriever::TimeRetriever()
{
  // Constructor implementation if needed
}

void TimeRetriever::initialize()
{
  // Initialize and synchronize time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Wait for time to sync
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println("Time synchronized");
}

String TimeRetriever::getCurrentTime()
{
  // Code to retrieve the current time
  return "12:34:56"; // Placeholder implementation
}

String TimeRetriever::getCurrentDate()
{
  // Code to retrieve the current date
  return "2023-10-01"; // Placeholder implementation
}

String TimeRetriever::getTimestamp()
{
  // Code to retrieve the current timestamp
  return getCurrentDate() + " " + getCurrentTime();
}

// #include <NTPClient.h>
// #include <TimeLib.h>

// WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP, "pool.ntp.org");

// //Converts the unix Timestamp into a readable format.
// String convertTimeStamp(unsigned long timestamp) {
//   // Convert timestamp to time_t
//   time_t rawtime = (time_t)timestamp;

//   // Obtain the current time
//   struct tm *timeinfo;
//   timeinfo = localtime(&rawtime);

//   // Create a string to hold the formatted date and time
//   char dateTimeString[20];

//   // Format the date and time
//   sprintf(dateTimeString, "%04d-%02d-%02d %02d:%02d:%02d",
//           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
//           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

//   // Convert the char array to a String and return it
//   return String(dateTimeString);
// }

// // Returns the current time as a Unix Timestamp For the Influx Database
// unsigned long getCurrentTime() {

// //Added to test if it stops hang on no internet connection
// if (!timeClient.update ()){
//   return 0;
// }
//   timeClient.update();
//   unsigned long timestamp = timeClient.getEpochTime();

//   // String convertedTimeString = convertTimeStamp(timestamp);

//   return timestamp;
// }

// //Returns the Current Date and Time as a Readable format YY:MM:DD HH:MM:SS
// String getCurrentReadableTime() {

//   timeClient.update();
//   unsigned long timestamp = timeClient.getEpochTime();

//   String convertedTimeString = convertTimeStamp(timestamp);

//   return convertedTimeString;
// }
