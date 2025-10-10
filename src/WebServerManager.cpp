// WebServerManager.cpp
// This file manages the web server and WebSocket communication for the ESP32.

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "managers/WebServerManager.h"
#include "managers/NotepadManager.h"
#include "utilities/FileSystemExplorer.h"
#include "config/Config.h"
#include "utilities/WebServerActions.h"
#include "utilities/SerialRemote.h"
#include "managers/StateManager.h"
#include <array>
// Define the static server members
AsyncWebServer WebServerManager::server(SERVER_PORT);
AsyncWebSocket WebServerManager::ws(WEBSOCKET_PATH);

// Define shared state
SystemState state;

// History buffer as circular buffer
std::array<HistoryEntry, HISTORY_SIZE> history = {}; // Only define here, declare as extern in header
std::array<EventEntry, MAX_EVENTS> events = {};
int historyIndex = 0;
int eventsCount = 0;

// Forward declarations of action handlers (must match signature)
AsyncWebServer &WebServerManager::getServer()
{
    return server; // Your internal AsyncWebServer instance
}

// Action map array and size
const WebServerManager::ActionMapping WebServerManager::actionMap[] = {
    {"controlUpdate", [](WebServerManager *mgr, AsyncWebSocketClient *client, JsonVariant data)
     { WebServerActions::handleControlUpdate(mgr, client, data); }},
    {"getHistory", [](WebServerManager *mgr, AsyncWebSocketClient *client, JsonVariant data)
     { WebServerActions::handleGetHistory(mgr, client, data); }},
    {"notepadList", [](WebServerManager *mgr, AsyncWebSocketClient *client, JsonVariant data)
     { WebServerActions::handleNotepadList(mgr, client, data); }},
    {"notepadLoad", [](WebServerManager *mgr, AsyncWebSocketClient *client, JsonVariant data)
     { WebServerActions::handleNotepadLoad(mgr, client, data); }},
    {"notepadSave", [](WebServerManager *mgr, AsyncWebSocketClient *client, JsonVariant data)
     { WebServerActions::handleNotepadSave(mgr, client, data); }},
    {"getConfig", [](WebServerManager *mgr, AsyncWebSocketClient *client, JsonVariant data) {
        StaticJsonDocument<512> configDoc;
        configDoc["tempSetpoint"] = state.tempSetpoint;
        configDoc["rpmSetpoint"] = state.rpmSetpoint;
        configDoc["alertTempThreshold"] = state.alertTempThreshold;
        configDoc["alertRpmThreshold"] = state.alertRpmThreshold;
        configDoc["alertTimerThreshold"] = state.alertTimerThreshold;
        String configJson;
        serializeJson(configDoc, configJson);
        client->text(configJson);
    }},
    {"resetSystem", [](WebServerManager *mgr, AsyncWebSocketClient *client, JsonVariant data) {
        logMessagef(LogLevel::INFO, "[WebServerManager] Resetting system...");
        ESP.restart();
    }},
    {"updateState", [](WebServerManager *mgr, AsyncWebSocketClient *client, JsonVariant json) {
        if (json.is<JsonObject>()) {
            JsonObject data = json.as<JsonObject>();
            if (data.containsKey("temperature") && data.containsKey("rpm") && data.containsKey("mode")) {
                float temperature = data["temperature"];
                int rpm = data["rpm"];
                String mode = data["mode"].as<String>();
                StateManager::updateState(temperature, rpm, mode, state.tempSetpoint, state.rpmSetpoint, state.duration, mgr->modeManager);
                mgr->notifyClients();
            } else {
                mgr->sendError(client, "Incomplete state data");
            }
        } else {
            mgr->sendError(client, "Missing 'data' field for state update");
        }
    }}
};
const int WebServerManager::actionMapSize = sizeof(WebServerManager::actionMap) / sizeof(WebServerManager::actionMap[0]);

// Constructor
WebServerManager::WebServerManager()
{
    initializeModeHandlers();
}

// Singleton accessor
WebServerManager *WebServerManager::instance()
{
    static WebServerManager inst;
    return &inst;
}

// --- Helper methods for WebSocket communication ---
void WebServerManager::sendAck(AsyncWebSocketClient *client, const String &message)
{
    StaticJsonDocument<128> doc;
    doc["type"] = "ack";
    doc["message"] = message;
    String json;
    serializeJson(doc, json);
    client->text(json);
}

void WebServerManager::sendError(AsyncWebSocketClient *client, const String &error)
{
    StaticJsonDocument<128> doc;
    doc["type"] = "error";
    doc["message"] = error;
    String json;
    serializeJson(doc, json);
    client->text(json);
}

void WebServerManager::sendJsonResponse(AsyncWebSocketClient *client, const JsonObject& response) {
    String jsonString;
    serializeJson(response, jsonString);
    if (client && client->status() == WS_CONNECTED) {
        client->text(jsonString);
    }
}

// --- Initialization methods ---
bool WebServerManager::beginWiFi(const char *ssid, const char *password)
{
    Serial.println(F("[WebServerManager] Connecting to WiFi..."));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000)
    {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(F("\n[WebServerManager] WiFi connection timeout."));
        return false;
    }

    Serial.println();
    Serial.printf("[WebServerManager] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

bool WebServerManager::beginFileSystem()
{
    if (!LittleFS.begin(true))
    {
        Serial.println(F("[WebServerManager] Failed to mount LittleFS!"));
        return false;
    }
    Serial.println(F("[WebServerManager] File system mounted."));
    return true;
}

void WebServerManager::beginServer()
{
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    ws.onEvent(onWsEventStatic);
    server.addHandler(&ws);

    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not Found"); });

    server.begin();
    Serial.println(F("[WebServerManager] Server started on port 80."));
}

// Main initialization method
void WebServerManager::begin(const char *ssid, const char *password)
{
    if (!beginWiFi(ssid, password))
    {
        Serial.println(F("[WebServerManager] WiFi connection failed."));
        return;
    }

    if (!beginFileSystem())
    {
        Serial.println(F("[WebServerManager] File system mount failed."));
        return;
    }

    beginServer();

    state.startTime = millis();
    Serial.println(F("[WebServerManager] Web server fully initialized."));
}

void WebServerManager::attachModeManager(HeaterModeManager *manager)
{
    Serial.println(F("[WebServerManager] Mode manager attached"));
    modeManager = manager;
}

void WebServerManager::handle()
{
    ws.cleanupClients();
}

void WebServerManager::notifyClients()
{
    if (ws.count() == 0)
        return;

    DynamicJsonDocument doc(512);
    doc["type"] = "dataUpdate";
    JsonObject data = doc.createNestedObject("data");

    data["temperature"] = state.temperature;
    data["rpm"] = state.rpm;
    data["mode"] = state.mode;
    data["temp_setpoint"] = state.tempSetpoint;
    data["rpm_setpoint"] = state.rpmSetpoint;
    data["duration"] = state.duration;
    data["alertTempThreshold"] = state.alertTempThreshold;
    data["alertRpmThreshold"] = state.alertRpmThreshold;
    data["alertTimerThreshold"] = state.alertTimerThreshold;
    data["running_time"] = (millis() - state.startTime) / 1000;
    String json;
    serializeJson(doc, json);
    ws.textAll(json);

    // Log the state after notifying clients
    StateManager::logState();
}

// WebSocket event handlers (static calls instance method)
void WebServerManager::onWsEventStatic(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                       AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    instance()->onWsEvent(server, client, type, arg, data, len);
}

void WebServerManager::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                 AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("Client %u connected via WebSocket\n", client->id());
        notifyClients();
        break;

    case WS_EVT_DISCONNECT:
        Serial.printf("Client %u disconnected\n", client->id());
        break;

    case WS_EVT_DATA:
        handleWebSocketMessage(client, data, len); // Restored call to handleWebSocketMessage
        break;

    default:
        break;
    }
}

// Improved WebSocket message handler with better error handling
void WebServerManager::handleWebSocketMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len)
{
    if (!modeManager)
    {
        Serial.println(F("[WebServerManager] modeManager is null!"));
        sendError(client, "Mode manager not attached");
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err)
    {
        Serial.printf("[WebServerManager] Failed to parse JSON: %s\n", err.c_str());
        sendError(client, "Invalid JSON format");
        return;
    }

    JsonVariant json = doc.as<JsonVariant>();
    if (json.containsKey("action")) {
        String action = json["action"].as<String>();
        Serial.printf("[WebServerManager] Received action: %s\n", action.c_str());

        // Use actionMap for dispatch
        for (int i = 0; i < actionMapSize; ++i) {
            if (action.equalsIgnoreCase(actionMap[i].action)) {
                JsonVariant dataVariant;
                if (json.containsKey("data")) {
                    dataVariant = JsonVariant(json["data"]);
                    Serial.print("[WebServerManager] Data for action: ");
                    serializeJsonPretty(dataVariant, Serial);
                } else {
                    dataVariant = JsonVariant(json);
                    Serial.println("[WebServerManager] No data provided for action");

                }
                
                Serial.print("[WebServerManager] Dispatching action with data: ");
                serializeJsonPretty(json, Serial);
                Serial.println();
                actionMap[i].handler(this, client, dataVariant);
                return;
            }
        }
        // Unknown action
        Serial.printf("[WebServerManager] Unknown action: %s\n", action.c_str());
        String errorMsg = "{\"type\":\"error\",\"message\":\"Unknown action: " + action + "\"}";
        client->text(errorMsg);
        return;
    } else {
        sendError(client, "Missing 'action' field");
        return;
    }
}

// Initialize mode handlers
void WebServerManager::initializeModeHandlers()
{
    // Initialize mode handlers with C-style string constants
    modeHandlers[Modes::OFF] = [this](const JsonObject &params)
    { this->handleModeOff(params); };
    modeHandlers[Modes::HOLD] = [this](const JsonObject &params)
    { this->handleModeHold(params); };
    modeHandlers[Modes::RAMP] = [this](const JsonObject &params)
    { this->handleModeRamp(params); };
    modeHandlers[Modes::TIMER] = [this](const JsonObject &params)
    { this->handleModeTimer(params); };
}

// Mode handler implementations
void WebServerManager::handleModeOff(const JsonObject &params)
{
    if (modeManager)
    {
        modeManager->setOff();
    }
}

void WebServerManager::handleModeHold(const JsonObject &params)
{
    if (modeManager)
    {
        modeManager->setHold(state.tempSetpoint);
    }
}

void WebServerManager::handleModeRamp(const JsonObject &params)
{
    if (!modeManager)
        return;

    float rampRate = params.containsKey("ramp_rate") ? params["ramp_rate"].as<float>() : 1.0f;
    modeManager->setRamp(state.tempSetpoint, rampRate, 9999);
}

void WebServerManager::handleModeTimer(const JsonObject &params)
{
    if (modeManager)
    {
        modeManager->setTimer(state.duration, state.tempSetpoint);
    }
}

// Handles controlUpdate action (mostly state and mode updates)
void WebServerManager::handleControlUpdate(AsyncWebSocketClient *client, JsonVariant data)
{
    if (!data.is<JsonObject>())
    {
        sendError(client, "Missing or invalid data field");
        return;
    }

    JsonObject obj = data.as<JsonObject>();
    bool stateChanged = false;

    float newTempSetpoint = state.tempSetpoint;
    int newRpmSetpoint = state.rpmSetpoint;
    String newMode = state.mode;
    int newDuration = state.duration;

    // Handle temperature setpoint
    if (obj.containsKey("temp_setpoint"))
    {
        newTempSetpoint = obj["temp_setpoint"].as<float>();
        stateChanged = true;
    }

    // Handle RPM setpoint
    if (obj.containsKey("rpm_setpoint"))
    {
        newRpmSetpoint = obj["rpm_setpoint"].as<int>();
        stateChanged = true;
    }

    // Handle mode changes
    if (obj.containsKey("mode"))
    {
        newMode = obj["mode"].as<String>();
        stateChanged = true;
    }

    // Handle duration changes
    if (obj.containsKey("duration"))
    {
        newDuration = obj["duration"].as<int>();
        stateChanged = true;
    }

    if (stateChanged)
    {
        StateManager::updateState(state.temperature, state.rpm, newMode, newTempSetpoint, newRpmSetpoint, newDuration, modeManager);
        state.startTime = millis();
        notifyClients();
    }
    sendAck(client, "Update received");
}

// Add a new entry to the history circular buffer
void WebServerManager::addHistoryEntry(float temperature)
{
    history[historyIndex].temperature = temperature;
    history[historyIndex].timestamp = millis(); // Store absolute time reference (milliseconds since boot)
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
}

// Handles getHistory action
void WebServerManager::handleGetHistory(AsyncWebSocketClient *client, JsonVariant)
{
    StaticJsonDocument<2048> histDoc;
    JsonArray arr = histDoc.createNestedArray("data");
    bool hasEntries = false;

    // Return history entries in chronological order starting from oldest
    int start = historyIndex; // oldest entry in circular buffer
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        int idx = (start + i) % HISTORY_SIZE;
        if (history[idx].timestamp == 0)
            continue;

        JsonObject entry = arr.createNestedObject();
        entry["time"] = history[idx].timestamp; // Send absolute timestamp (millis)
        entry["temperature"] = history[idx].temperature;
        hasEntries = true;
    }

    if (!hasEntries)
    {
        sendError(client, "No history data available");
        return;
    }

    histDoc["type"] = "history";

    String jsonStr;
    if (serializeJson(histDoc, jsonStr) == 0)
    {
        sendError(client, "Failed to serialize history data");
        return;
    }

    client->text(jsonStr);
}

// Placeholder handlers for Notepad API via WebSocket (if you want to extend)
void WebServerManager::handleNotepadList(AsyncWebSocketClient *client, JsonVariant json)
{
    Serial.println("[WebServerManager] handleNotepadList called");

    StaticJsonDocument<1024> doc;
    doc["type"] = "notepadList";

    JsonArray arr = doc.createNestedArray("experiments");

    // listNotes is void, so we don't check its return value
    NotepadManager::getInstance().listNotes(arr);

    String out;
    if (serializeJson(doc, out) == 0)
    {
        sendError(client, "Failed to serialize note list");
        return;
    }

    client->text(out);
}

void WebServerManager::handleNotepadLoad(AsyncWebSocketClient *client, JsonVariant data)
{
    if (!data.is<JsonObject>() || !data.containsKey("experiment"))
    {
        sendError(client, "Missing experiment parameter");
        return;
    }

    String experiment = data["experiment"].as<String>();
    String notes;

    // Assuming loadNote is void and populates notes by reference
    NotepadManager::getInstance().loadNote(experiment, notes);

    StaticJsonDocument<512> doc;
    doc["type"] = "notepadData";
    doc["experiment"] = experiment;
    doc["notes"] = notes;

    String json;
    if (serializeJson(doc, json) == 0)
    {
        sendError(client, "Failed to serialize note data");
        return;
    }

    client->text(json);
}

void WebServerManager::handleNotepadSave(AsyncWebSocketClient *client, JsonVariant data)
{
    String jsonStr;
    serializeJson(data, jsonStr);
    Serial.printf("[WebServerManager] handleNotepadSave called with data: %s\n", jsonStr.c_str());

    if (!data.is<JsonObject>() || !data.containsKey("experiment") || !data.containsKey("notes"))
    {
        sendError(client, "Missing experiment or notes parameter");
        return;
    }

    String experiment = data["experiment"].as<String>();
    String notes = data["notes"].as<String>();

    // Assuming saveNote is void, otherwise adjust accordingly
    NotepadManager::getInstance().saveNote(experiment, notes);
    sendAck(client, "Note saved successfully");
}

// === Utility methods ===

void WebServerManager::updateStateProperty(float &var, float val, const char *name)
{
    if (val != var)
    {
        logEvent(String(name) + " changed from " + String(var) + " to " + String(val));
        var = val;
    }
}

void WebServerManager::updateStateProperty(int &var, int val, const char *name)
{
    if (val != var)
    {
        logEvent(String(name) + " changed from " + String(var) + " to " + String(val));
        var = val;
    }
}

void WebServerManager::updateMode(const String &newMode)
{
    if (newMode != state.mode &&
        (newMode == Modes::HOLD ||
         newMode == Modes::RAMP ||
         newMode == Modes::RECRYSTALLIZATION ||
         newMode == Modes::TIMER ||
         newMode == Modes::OFF))
    {
        logEvent("Mode changed from " + state.mode + " to " + newMode);
        state.mode = newMode;
    }
}

void WebServerManager::logEvent(const String &desc)
{
    if (eventsCount == MAX_EVENTS)
    {
        // Shift all events left by one (remove the oldest)
        for (int i = 0; i < MAX_EVENTS - 1; i++)
        {
            events[i] = events[i + 1];
        }
        eventsCount--;
    }

    // Add the new event at the end
    events[eventsCount].timestamp = millis();
    events[eventsCount].description = desc;
    eventsCount++;
}

// Add this line near other static/global variables (after server/ws definitions)
