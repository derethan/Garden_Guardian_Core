/*************************************************
*     Function to get time from NTMP Serves
*     Function to Convert Time to String (YY:MM:DD:HH:MM:SS)
************************************************/

#include <NTPClient.h>
#include <TimeLib.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

String convertTimeStamp(unsigned long timestamp) {
  // Convert timestamp to time_t
  time_t rawtime = (time_t)timestamp;

  // Obtain the current time
  struct tm *timeinfo;
  timeinfo = localtime(&rawtime);

  // Create a string to hold the formatted date and time
  char dateTimeString[20];

  // Format the date and time
  sprintf(dateTimeString, "%04d-%02d-%02d %02d:%02d:%02d",
          timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
          timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  // Convert the char array to a String and return it
  return String(dateTimeString);
}


String getCurrentTime() {

  timeClient.update();
  unsigned long timestamp = timeClient.getEpochTime();

  String convertedTimeString = convertTimeStamp(timestamp);

  return convertedTimeString;
}
