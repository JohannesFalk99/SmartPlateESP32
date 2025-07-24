#pragma once
#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <unordered_map>
#include <functional>
#include "HeaterModeManager.h"
#include "NotepadManager.h"
#include "config/Config.h"

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
    String mode = Modes::HOLD;

    int duration = 0;

    float alertTempThreshold = ALERT_TEMP_THRESHOLD;
    float alertRpmThreshold = ALERT_RPM_THRESHOLD;
    int alertTimerThreshold = ALERT_TIMER_THRESHOLD;

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
    // Action handler functions
    void handleControlUpdate(AsyncWebSocketClient *client, JsonVariant data);

    void handleGetHistory(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadList(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadLoad(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadSave(AsyncWebSocketClient *client, JsonVariant data);

private:
    // Web server and websocket instances
    static AsyncWebServer server;
    static AsyncWebSocket ws;

    HeaterModeManager *modeManager = nullptr;

    // Helper methods
    void sendAck(AsyncWebSocketClient *client, const String &message);
    void sendError(AsyncWebSocketClient *client, const String &error);

    void sendJsonResponse(AsyncWebSocketClient *client, const JsonObject &response);

    // Initialization methods
    bool beginWiFi(const char *ssid, const char *password);
    bool beginFileSystem();
    void beginServer();

    // WebSocket event handlers
    static void onWsEventStatic(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                AwsEventType type, void *arg, uint8_t *data, size_t len);
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len);

    // Main WebSocket message handler (dispatches actions)
    void handleWebSocketMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len);

    // === Action handler dispatch system ===
    using ActionHandler = std::function<void(WebServerManager*, AsyncWebSocketClient*, JsonVariant)>;

    struct ActionMapping {
        const char* action;
        ActionHandler handler;
    };

    static const ActionMapping actionMap[];
    static const int actionMapSize;

   

    // Mode handler function type
    using ModeHandler = std::function<void(const JsonObject&)>;
    
    // Mode handler methods
    void handleModeOff(const JsonObject& params);
    void handleModeHold(const JsonObject& params);
    void handleModeRamp(const JsonObject& params);
    void handleModeTimer(const JsonObject& params);
    
    // Initialize mode handlers
    void initializeModeHandlers();
    
    // Helper functions to update state and log events
    void updateStateProperty(float &var, float val, const char *name);
    void updateStateProperty(int &var, int val, const char *name);
    void updateMode(const String &newMode);
    void logEvent(const String &desc);
    
    // Mode handler map with const char* as key (using C-string comparison)
    struct CStringHash {
        std::size_t operator()(const char* str) const {
            std::size_t hash = 0;
            while (*str) {
                hash = hash * 101 + *str++;
            }
            return hash;
        }
    };

    struct CStringEqual {
        bool operator()(const char* a, const char* b) const {
            return strcmp(a, b) == 0;
        }
    };

    std::unordered_map<const char*, ModeHandler, CStringHash, CStringEqual> modeHandlers;
};

// Global shared state
extern SystemState state;

extern int historyIndex;


extern int eventsCount;

#endif // WEBSERVERMANAGER_H
