#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

extern WiFiServer serialServer;
extern WiFiClient serialClient;

enum class LogLevel { INFO, ERROR, DEBUG };
void logMessage(LogLevel level, const char* message);
void logMessagef(LogLevel level, const char* fmt, ...);
void handleRemoteSerial();
void setupRemoteSerial(int port = 23);