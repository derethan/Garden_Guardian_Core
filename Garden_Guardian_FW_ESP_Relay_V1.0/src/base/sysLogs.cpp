/**
 * @file sysLogs.cpp
 * @brief Implementation of system logging functionality with ANSI color support
 * 
 * Provides color-coded logging functions for different severity levels and purposes.
 * All functions respect the DEBUG_MODE flag except serialLog() which is designed
 * for user interaction during SERIAL_MODE configuration.
 */

#include "base/sysLogs.h"
#include "state.h"

/**
 * @brief Print a formatted section header for organizing log output
 */
void SysLogs::printSectionHeader(const String &header)
{
    if (DEBUG_MODE)
    {
        Serial.println();
        Serial.print(Colors::BRIGHT_CYAN);
        Serial.print("===== ");
        Serial.print(header);
        Serial.print(" =====");
        Serial.println(Colors::RESET);
    }
}

/**
 * @brief Log a critical error message with red color
 * @note Always updates system state error tracking regardless of DEBUG_MODE
 */
void SysLogs::logError(const String &message)
{
    state.lastErrorMessage = message;
    state.lastErrorTime = state.currentTime;
    if (DEBUG_MODE)
    {
        Serial.print(Colors::BRIGHT_RED);
        Serial.print("[ERROR]: ");
        Serial.print(Colors::RESET);
        Serial.println(message);
    }
}

/**
 * @brief Log a warning message with yellow color
 */
void SysLogs::logWarning(const String &message)
{
    if (DEBUG_MODE)
    {
        Serial.print(Colors::BRIGHT_YELLOW);
        Serial.print("[WARNING]: ");
        Serial.print(Colors::RESET);
        Serial.println(message);
    }
}

/**
 * @brief Log an informational message with cyan color for the message type
 */
void SysLogs::logInfo(const String &messageType, const String &message)
{
    if (DEBUG_MODE)
    {
        Serial.print(Colors::CYAN);
        Serial.print("[ ");
        Serial.print(messageType);
        Serial.print("]: ");
        Serial.print(Colors::RESET);
        Serial.println(message);
    }
}

/**
 * @brief Log a debug message with magenta color for detailed troubleshooting
 */
void SysLogs::logDebug(const String &messageType, const String &message)
{
    if (DEBUG_MODE)
    {
        Serial.print(Colors::MAGENTA);
        Serial.print("[DEBUG-");
        Serial.print(messageType);
        Serial.print("]: ");
        Serial.print(Colors::RESET);
        Serial.println(message);
    }
}

/**
 * @brief Log a verbose/trace message with bright blue color for execution flow tracking
 */
void SysLogs::logTrace(const String &messageType, const String &message)
{
    if (DEBUG_MODE)
    {
        Serial.print(Colors::BRIGHT_BLUE);
        Serial.print("[TRACE-");
        Serial.print(messageType);
        Serial.print("]: ");
        Serial.print(Colors::RESET);
        Serial.println(message);
    }
}

/**
 * @brief Log a success message with green color
 */
void SysLogs::logSuccess(const String &messageType, const String &message)
{
    if (DEBUG_MODE)
    {
        Serial.print(Colors::BRIGHT_GREEN);
        Serial.print("[SUCCESS-");
        Serial.print(messageType);
        Serial.print("]: ");
        Serial.print(Colors::RESET);
        Serial.println(message);
    }
}

/**
 * @brief Log a message in SERIAL_MODE regardless of DEBUG_MODE setting
 * @note This function ALWAYS outputs, even when DEBUG_MODE is false
 * @note Use ONLY when system is in SystemMode::SERIAL_MODE for user interaction
 */
void SysLogs::serialLog(const String &message)
{
    // Always output, regardless of DEBUG_MODE
    // This is specifically for user interaction during serial configuration
    Serial.println(message);
}

/**
 * @brief Print a simple message without formatting
 */
void SysLogs::print(const String &message)
{
    if (DEBUG_MODE)
    {
        Serial.print(message);
    }
}

/**
 * @brief Print a simple message with newline without formatting
 */
void SysLogs::println(const String &message)
{
    if (DEBUG_MODE)
    {
        Serial.println(message);
    }
}

/**
 * @brief Print an empty line
 */
void SysLogs::println()
{
    if (DEBUG_MODE)
    {
        Serial.println();
    }
}