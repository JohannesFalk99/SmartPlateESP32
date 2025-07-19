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

// Data structures

struct HistoryEntry
{
    unsigned long timestamp;
    float temperature;
};

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

struct EventEntry
{
    unsigned long timestamp;
    String description;
};

class WebServerManager
{
public:
    WebServerManager();

    // Singleton accessor
    static WebServerManager *instance();

    static AsyncWebServer &getServer();
    
    // Core public interface
    void begin(const char *ssid, const char *password);
    void handle();
    void notifyClients();

    // Attach external mode manager dependency
    void attachModeManager(HeaterModeManager *manager);
    void addHistoryEntry(float temperature);

private:
    // Web server and websocket instances
    static AsyncWebServer server;
    static AsyncWebSocket ws;

    HeaterModeManager *modeManager = nullptr;

    // WebSocket event handlers
    static void onWsEventStatic(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                AwsEventType type, void *arg, uint8_t *data, size_t len);
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len);

    // Main WebSocket message handler (dispatches actions)
    void handleWebSocketMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len);

    // === Action handler dispatch system ===
    typedef void (WebServerManager::*ActionHandler)(AsyncWebSocketClient *client, JsonVariant data);

    struct ActionMapping
    {
        const char *actionName;
        ActionHandler handler;
    };

    static const ActionMapping actionMap[];
    static const int actionMapSize;

    // Action handler functions
    void handleControlUpdate(AsyncWebSocketClient *client, JsonVariant data);
   
    void handleGetHistory(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadList(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadLoad(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadSave(AsyncWebSocketClient *client, JsonVariant data);

    // Helper functions to update state and log events
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
