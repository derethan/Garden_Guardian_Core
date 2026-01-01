#ifndef TIMEZONES_H
#define TIMEZONES_H

/**
 * @file timezones.h
 * @brief Comprehensive timezone definitions with human-readable names
 * 
 * Provides a mapping between user-friendly timezone names and their
 * POSIX TZ format strings for use with ESP32 time functions.
 * 
 * Format: {Human Readable Name, POSIX TZ String}
 */

struct TimezoneInfo {
    const char* name;
    const char* tzString;
};

// Comprehensive list of timezones with human-readable names
const TimezoneInfo TIMEZONES[] = {
    // UTC
    {"UTC", "UTC0"},
    {"Coordinated Universal Time", "UTC0"},
    
    // North America - United States
    {"US Eastern Time", "EST5EDT,M3.2.0,M11.1.0"},
    {"US Central Time", "CST6CDT,M3.2.0,M11.1.0"},
    {"US Mountain Time", "MST7MDT,M3.2.0,M11.1.0"},
    {"US Pacific Time", "PST8PDT,M3.2.0,M11.1.0"},
    {"US Alaska Time", "AKST9AKDT,M3.2.0,M11.1.0"},
    {"US Hawaii Time", "HST10"},
    {"US Arizona Time", "MST7"},  // Arizona doesn't observe DST
    
    // North America - Canada
    {"Canada Atlantic Time", "AST4ADT,M3.2.0,M11.1.0"},
    {"Canada Newfoundland Time", "NST3:30NDT,M3.2.0,M11.1.0"},
    
    // Central & South America
    {"Mexico City Time", "CST6CDT,M4.1.0,M10.5.0"},
    {"Argentina Time", "<-03>3"},
    {"Brazil Time (São Paulo)", "<-03>3"},
    {"Chile Time", "<-04>4<-03>,M9.1.6/24,M4.1.6/24"},
    {"Colombia Time", "<-05>5"},
    {"Peru Time", "<-05>5"},
    
    // Europe
    {"UK Time", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Ireland Time", "IST-1GMT0,M10.5.0,M3.5.0/1"},
    {"Central European Time", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Eastern European Time", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Western European Time", "WET0WEST,M3.5.0/1,M10.5.0"},
    {"Moscow Time", "<+03>-3"},
    
    // Asia
    {"India Time", "IST-5:30"},
    {"China Time", "CST-8"},
    {"Japan Time", "JST-9"},
    {"Korea Time", "KST-9"},
    {"Hong Kong Time", "HKT-8"},
    {"Singapore Time", "SGT-8"},
    {"Thailand Time", "ICT-7"},
    {"Vietnam Time", "ICT-7"},
    {"Indonesia Western Time", "WIB-7"},
    {"Indonesia Central Time", "WITA-8"},
    {"Indonesia Eastern Time", "WIT-9"},
    {"Philippines Time", "PST-8"},
    {"Pakistan Time", "PKT-5"},
    {"Bangladesh Time", "BST-6"},
    {"Nepal Time", "NPT-5:45"},
    {"Sri Lanka Time", "IST-5:30"},
    {"Myanmar Time", "MMT-6:30"},
    {"Dubai Time", "GST-4"},
    {"Israel Time", "IST-2IDT,M3.4.4/26,M10.5.0"},
    {"Turkey Time", "<+03>-3"},
    
    // Australia & Pacific
    {"Australia Western Time", "AWST-8"},
    {"Australia Central Time", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
    {"Australia Eastern Time", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"New Zealand Time", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
    {"Fiji Time", "<+12>-12<+13>,M11.2.0,M1.2.6/99"},
    {"Hawaii Time", "HST10"},
    
    // Africa
    {"South Africa Time", "SAST-2"},
    {"Egypt Time", "EET-2"},
    {"West Africa Time", "WAT-1"},
    {"East Africa Time", "EAT-3"},
    {"Central Africa Time", "CAT-2"},
    
    // Atlantic
    {"Azores Time", "<-01>1<+00>,M3.5.0/0,M10.5.0/1"},
    {"Cape Verde Time", "<-01>1"},
    {"Iceland Time", "GMT0"},
};

// Number of timezones in the array
const int TIMEZONES_COUNT = sizeof(TIMEZONES) / sizeof(TIMEZONES[0]);

/**
 * @brief Find timezone string by human-readable name
 * @param name The human-readable timezone name to search for
 * @return The POSIX TZ string if found, "UTC0" if not found
 */
inline const char* getTimezoneString(const char* name) {
    for (int i = 0; i < TIMEZONES_COUNT; i++) {
        if (strcmp(TIMEZONES[i].name, name) == 0) {
            return TIMEZONES[i].tzString;
        }
    }
    return "UTC0";  // Default to UTC if not found
}

/**
 * @brief Find timezone name by POSIX TZ string
 * @param tzString The POSIX TZ string to search for
 * @return The human-readable name if found, "UTC" if not found
 */
inline const char* getTimezoneName(const char* tzString) {
    for (int i = 0; i < TIMEZONES_COUNT; i++) {
        if (strcmp(TIMEZONES[i].tzString, tzString) == 0) {
            return TIMEZONES[i].name;
        }
    }
    return "UTC";  // Default to UTC if not found
}

#endif // TIMEZONES_H
