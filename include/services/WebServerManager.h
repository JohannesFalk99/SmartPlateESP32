#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <unordered_map>
#include <functional>
#include <array>
#include "HeaterModeManager.h"
#include "NotepadManager.h"

struct HistoryEntry { unsigned long timestamp; float temperature; };
struct EventEntry { unsigned long timestamp; String description; };
struct SystemState {
    float temperature = 0.0; int rpm = 0; float tempSetpoint = 0.0; int rpmSetpoint = 0; String mode = "HOLD"; int duration = 0;
    float alertTempThreshold = 0; float alertRpmThreshold = 0; int alertTimerThreshold = 0; unsigned long startTime = 0; };
extern SystemState state;
extern std::array<HistoryEntry, 256> history;
extern std::array<EventEntry, 64> events;
extern int historyIndex;
extern int eventsCount;

class WebServerManager {
public:
    WebServerManager();
    static WebServerManager *instance();
    static AsyncWebServer &getServer();
    void begin(const char *ssid, const char *password);
    void handle();
    void notifyClients();
    void attachModeManager(HeaterModeManager *manager);
    void addHistoryEntry(float temperature);
    void handleControlUpdate(AsyncWebSocketClient *client, JsonVariant data);
    void handleGetHistory(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadList(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadLoad(AsyncWebSocketClient *client, JsonVariant data);
    void handleNotepadSave(AsyncWebSocketClient *client, JsonVariant data);
private:
    static AsyncWebServer server;
    static AsyncWebSocket ws;
    HeaterModeManager *modeManager = nullptr;
};
