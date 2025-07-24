#include "SerialRemote.h"
#include <stdarg.h>
#include <WiFi.h> // Add this include

WiFiServer serialServer(23);
WiFiClient serialClient;

void setupRemoteSerial(int port) {
    serialServer.begin(port);
    serialServer.setNoDelay(true);
}

void handleRemoteSerial() {
    if (!serialClient || !serialClient.connected()) {
        WiFiClient newClient = serialServer.available();
        if (newClient) {
            serialClient = newClient;
            Serial.println("[SerialServer] Client connected");
        }
    }
    static unsigned long lastMsg = 0;
    if (serialClient && serialClient.connected() && millis() - lastMsg > 1000) {
        serialClient.println("[ESP32] Remote TCP debug: SmartPlate is running!");
        lastMsg = millis();
    }
    if (serialClient && serialClient.connected() && serialClient.available()) {
        char c = serialClient.read();
        Serial.write(c);
    }
}

void logMessage(LogLevel level, const char* message) {
    const char* levelStr = "";
    switch (level) {
        case LogLevel::INFO: levelStr = "[INFO]"; break;
        case LogLevel::ERROR: levelStr = "[ERROR]"; break;
        case LogLevel::DEBUG: levelStr = "[DEBUG]"; break;
    }
    String logLine = String(levelStr) + " " + message;
    Serial.println(logLine);
    if (serialClient && serialClient.connected()) {
        serialClient.println(logLine);
    }
}

void logMessagef(LogLevel level, const char* fmt, ...) {
    const char* levelStr = "";
    switch (level) {
        case LogLevel::INFO: levelStr = "[INFO]"; break;
        case LogLevel::ERROR: levelStr = "[ERROR]"; break;
        case LogLevel::DEBUG: levelStr = "[DEBUG]"; break;
    }
    char msgBuf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
    va_end(args);
    String logLine = String(levelStr) + " " + msgBuf;
    Serial.println(logLine);
    if (serialClient && serialClient.connected()) {
        serialClient.println(logLine);
    }
}
