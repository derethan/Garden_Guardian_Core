#ifndef BASE_SYSLOGS_H
#define BASE_SYSLOGS_H

#include <Arduino.h>

/**
 * @file sysLogs.h
 * @brief System logging interface with ANSI color support and various log levels
 * 
 * This module provides a comprehensive logging system for firmware applications
 * with color-coded output and multiple log levels. All logging functions respect
 * the DEBUG_MODE flag except for serialLog() which is specifically designed for
 * user interaction in SERIAL_MODE.
 */

/**
 * @namespace SysLogs
 * @brief Namespace containing all system logging functionality
 */
namespace SysLogs
{
    // ANSI color codes for terminal output
    namespace Colors
    {
        const char* const RESET = "\033[0m";
        const char* const RED = "\033[31m";
        const char* const GREEN = "\033[32m";
        const char* const YELLOW = "\033[33m";
        const char* const BLUE = "\033[34m";
        const char* const MAGENTA = "\033[35m";
        const char* const CYAN = "\033[36m";
        const char* const WHITE = "\033[37m";
        const char* const BRIGHT_RED = "\033[91m";
        const char* const BRIGHT_GREEN = "\033[92m";
        const char* const BRIGHT_YELLOW = "\033[93m";
        const char* const BRIGHT_BLUE = "\033[94m";
        const char* const BRIGHT_MAGENTA = "\033[95m";
        const char* const BRIGHT_CYAN = "\033[96m";
    }

    /**
     * @brief Log a critical error message (always updates state error tracking)
     * @param message The error message to log
     * 
     * Logs with red color. Only displays to Serial if DEBUG_MODE is enabled.
     * Always updates system state with error information regardless of DEBUG_MODE.
     */
    void logError(const String &message);

    /**
     * @brief Log a warning message
     * @param message The warning message to log
     * 
     * Logs with yellow color. Only displays to Serial if DEBUG_MODE is enabled.
     */
    void logWarning(const String &message);

    /**
     * @brief Log an informational message with a category/type
     * @param messageType The category or type of the message (e.g., "SYSTEM", "NETWORK")
     * @param message The informational message to log
     * 
     * Logs with cyan color for type, white for message. Only displays if DEBUG_MODE is enabled.
     */
    void logInfo(const String &messageType, const String &message);

    /**
     * @brief Log a debug message for detailed troubleshooting
     * @param messageType The category or type of debug message
     * @param message The debug message to log
     * 
     * Logs with magenta color. Only displays to Serial if DEBUG_MODE is enabled.
     * Use for detailed diagnostic information during development.
     */
    void logDebug(const String &messageType, const String &message);

    /**
     * @brief Log a verbose/trace message for very detailed execution flow
     * @param messageType The category or type of trace message
     * @param message The trace message to log
     * 
     * Logs with bright blue color. Only displays to Serial if DEBUG_MODE is enabled.
     * Use for extremely detailed execution tracking.
     */
    void logTrace(const String &messageType, const String &message);

    /**
     * @brief Log a success message
     * @param messageType The category or type of success message
     * @param message The success message to log
     * 
     * Logs with green color. Only displays to Serial if DEBUG_MODE is enabled.
     * Use for confirming successful operations.
     */
    void logSuccess(const String &messageType, const String &message);

    /**
     * @brief Log a message in SERIAL_MODE regardless of DEBUG_MODE setting
     * @param message The message to display to the user
     * 
     * This function always outputs to Serial, even when DEBUG_MODE is false.
     * Intended for user interaction during serial configuration mode to prevent
     * message flooding when configuring the device via serial connection.
     * 
     * Use this ONLY when the system is in SystemMode::SERIAL_MODE for interactive
     * user configuration and menu display.
     */
    void serialLog(const String &message);

    /**
     * @brief Print a formatted section header for organizing log output
     * @param header The header text to display
     * 
     * Prints a visually distinct section header. Only displays if DEBUG_MODE is enabled.
     */
    void printSectionHeader(const String &header);

    /**
     * @brief Print a simple message without formatting
     * @param message The message to print
     * 
     * Basic output function. Only displays to Serial if DEBUG_MODE is enabled.
     */
    void print(const String &message);

    /**
     * @brief Print a simple message with newline without formatting
     * @param message The message to print
     * 
     * Basic output function with newline. Only displays to Serial if DEBUG_MODE is enabled.
     */
    void println(const String &message);

    /**
     * @brief Print an empty line
     * 
     * Outputs a blank line. Only displays to Serial if DEBUG_MODE is enabled.
     */
    void println();
}

#endif // BASE_SYSLOGS_H