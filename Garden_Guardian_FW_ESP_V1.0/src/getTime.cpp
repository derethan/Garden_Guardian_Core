/*************************************************
 *     Function to get time from NTP Serves
 *     Function to Convert Time to String (YY:MM:DD:HH:MM:SS)
 ************************************************/
#include <WiFi.h>
#include <time.h>
#include "getTime.h"

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -12600;        // Adjust for your timezone (e.g., -18000 for EST)
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

  // print formatted timeinfo to serial monitor
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

String TimeRetriever::getCurrentTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "00:00:00"; // Placeholder implementation
  }

  char timeString[9];
  strftime(timeString, 9, "%H:%M:%S", &timeinfo);

  return String(timeString);
}

String TimeRetriever::getCurrentDate()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "00:00:00"; // Placeholder implementation
  }

  char dateString[11];
  strftime(dateString, 11, "%Y-%m-%d", &timeinfo);

  return String(dateString);
}

String TimeRetriever::getTimestamp()
{
  // Code to retrieve the current timestamp
  return getCurrentDate() + " " + getCurrentTime();
}

// function to return the unix timestamp
unsigned long TimeRetriever::getUnixTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return 0; // Placeholder implementation
  }

  time_t time = mktime(&timeinfo);
  return (unsigned long)time;
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
