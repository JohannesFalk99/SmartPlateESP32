// #pragma once
// #ifndef WEBSERVERMANAGER_H
// #define WEBSERVERMANAGER_H

// #include <Arduino.h>
// #include <WiFi.h>
// #include <AsyncTCP.h>
// #include <ESPAsyncWebServer.h>
// #include <ArduinoJson.h>
// #include <LittleFS.h>
// #include "HeaterModeManager.h" // include this
// // Constants
// #define HISTORY_SIZE 300
// #define MAX_EVENTS 100

// // Data structure to store temperature history
// struct HistoryEntry
// {
//     unsigned long timestamp;
//     float temperature;
// };

// // Data structure for system state
// struct SystemState
// {
//     float temperature = 0.0;
//     int rpm = 0;

//     float tempSetpoint = 0.0;
//     int rpmSetpoint = 0;
//     String mode = "Hold";

//     int duration = 0;

//     float alertTempThreshold = 0.0;
//     float alertRpmThreshold = 0.0;
//     int alertTimerThreshold = 0;

//     unsigned long startTime = 0;
// };

// // Event log entry
// struct EventEntry
// {
//     unsigned long timestamp;
//     String description;
// };

// class WebServerManager
// {
// public:
//     WebServerManager();

//     static WebServerManager *instance();

//     void begin(const char *ssid, const char *password);
//     void handle();
//     void notifyClients();
//     void attachModeManager(HeaterModeManager *manager); // <-- Add this
// private:
//     static AsyncWebServer server;
//     static AsyncWebSocket ws;
//     HeaterModeManager *modeManager = nullptr; // <-- Add this
//     static void onWsEventStatic(AsyncWebSocket *server, AsyncWebSocketClient *client,
//                                 AwsEventType type, void *arg, uint8_t *data, size_t len);
//     void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
//                    AwsEventType type, void *arg, uint8_t *data, size_t len);

//     void handleWebSocketMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len);

//     void updateStateProperty(float &var, float val, const char *name);
//     void updateStateProperty(int &var, int val, const char *name);
//     void updateMode(const String &newMode);
//     void logEvent(const String &desc);
// };

// // Global shared state
// extern SystemState state;
// extern HistoryEntry history[HISTORY_SIZE];
// extern int historyIndex;

// extern EventEntry events[MAX_EVENTS];
// extern int eventsCount;

// #endif // WEBSERVERMANAGER_H
#pragma once
#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "HeaterModeManager.h"
#include "NotepadManager.h"

// Constants
#define HISTORY_SIZE 300
#define MAX_EVENTS 100

// Data structure to store temperature history
struct HistoryEntry
{
    unsigned long timestamp;
    float temperature;
};

// Data structure for system state
struct SystemState
{
    float temperature = 0.0;
    int rpm = 0;

    float tempSetpoint = 0.0;
    int rpmSetpoint = 0;
    String mode = "Hold";

    int duration = 0;

    float alertTempThreshold = 0.0;
    float alertRpmThreshold = 0.0;
    int alertTimerThreshold = 0;

    unsigned long startTime = 0;
};

// Event log entry
struct EventEntry
{
    unsigned long timestamp;
    String description;
};

class WebServerManager
{
public:
    WebServerManager();

    static WebServerManager *instance();

    void begin(const char *ssid, const char *password);
    void handle();
    void notifyClients();
    void attachModeManager(HeaterModeManager *manager);

private:
    static AsyncWebServer server;
    static AsyncWebSocket ws;

    HeaterModeManager *modeManager = nullptr;

    static void onWsEventStatic(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                AwsEventType type, void *arg, uint8_t *data, size_t len);
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len);

    void handleWebSocketMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len);

    void updateStateProperty(float &var, float val, const char *name);
    void updateStateProperty(int &var, int val, const char *name);
    void updateMode(const String &newMode);
    void logEvent(const String &desc);
};

// Global shared state
extern SystemState state;
extern HistoryEntry history[HISTORY_SIZE];
extern int historyIndex;

extern EventEntry events[MAX_EVENTS];
extern int eventsCount;

#endif // WEBSERVERMANAGER_H
