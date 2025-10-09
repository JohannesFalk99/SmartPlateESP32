#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

extern WiFiServer serialServer;
extern WiFiClient serialClient;

/**
 * @brief Log level enumeration for message severity
 */
enum class LogLevel { 
    INFO,   ///< Informational messages
    ERROR,  ///< Error messages
    DEBUG   ///< Debug messages
};

/**
 * @brief Log a message with specified severity level
 * @param level Severity level of the message
 * @param message Message text to log
 */
void logMessage(LogLevel level, const char* message);

/**
 * @brief Log a formatted message with specified severity level
 * @param level Severity level of the message
 * @param fmt Format string (printf style)
 * @param ... Variable arguments for format string
 */
void logMessagef(LogLevel level, const char* fmt, ...);

/**
 * @brief Handle remote serial connections and data
 * 
 * Call this regularly to process incoming connections and data
 * from remote serial clients.
 */
void handleRemoteSerial();

/**
 * @brief Setup remote serial server on specified port
 * @param port TCP port number to listen on (default 23 for telnet)
 */
void setupRemoteSerial(int port = 23);