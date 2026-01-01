/*************************************************
 *     Function to get time from NTP Serves
 *     Function to Convert Time to String (YY:MM:DD:HH:MM:SS)
 ************************************************/

#include "getTime.h"

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -12600; // Adjust for your timezone (e.g., -18000 for EST)
const int daylightOffset_sec = 0;  // Daylight savings time offset

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

  // Offset the time by -2.5 hours
  timeinfo.tm_hour -= 2;
  timeinfo.tm_min -= 30;
  timeinfo.tm_sec -= 0;
  mktime(&timeinfo); // Normalize the time structure

  // 24-hour format
  char timeString24[9];
  strftime(timeString24, sizeof(timeString24), "%H:%M:%S", &timeinfo);

  // 12-hour format with AM/PM
  char timeString12[12];
  strftime(timeString12, sizeof(timeString12), "%I:%M:%S %p", &timeinfo);

  // Print both formats with identifier
  Serial.print("[Time Retriever] 24-hour: ");
  Serial.println(timeString24);
  Serial.print("[Time Retriever] 12-hour: ");
  Serial.println(timeString12);

  return String(timeString24); // Return 24-hour format
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
