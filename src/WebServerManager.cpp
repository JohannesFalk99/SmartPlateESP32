#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "WebServerManager.h"
#include "NotepadManager.h"
#include "Explorer.h"



// Define the static server members
AsyncWebServer WebServerManager::server(80);
AsyncWebSocket WebServerManager::ws("/ws");

// Define shared state
SystemState state;

// History buffer as circular buffer
HistoryEntry history[HISTORY_SIZE] = {};
int historyIndex = 0;

EventEntry events[MAX_EVENTS] = {};
int eventsCount = 0;

// Forward declarations of action handlers (must match signature)
AsyncWebServer &WebServerManager::getServer()
{
    return server; // Your internal AsyncWebServer instance
}

// Action map array and size
const WebServerManager::ActionMapping WebServerManager::actionMap[] = {
    {"controlUpdate", &WebServerManager::handleControlUpdate},
    {"getHistory", &WebServerManager::handleGetHistory},
    {"notepadList", &WebServerManager::handleNotepadList},
    {"notepadLoad", &WebServerManager::handleNotepadLoad},
    {"notepadSave", &WebServerManager::handleNotepadSave},
};
const int WebServerManager::actionMapSize = sizeof(WebServerManager::actionMap) / sizeof(WebServerManager::actionMap[0]);

// Constructor
WebServerManager::WebServerManager() {}

// Singleton accessor
WebServerManager *WebServerManager::instance()
{
    static WebServerManager inst;
    return &inst;
}

// Initialize server and websocket, attach REST endpoints
void WebServerManager::begin(const char *ssid, const char *password)
{
    Serial.println(F("Connecting to WiFi..."));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
    }
    Serial.println();
    Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());

    if (!LittleFS.begin(true))
    {
        Serial.println(F("Failed to mount LittleFS!"));
        return;
    }

    // Serve static files
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // Setup WebSocket event
    ws.onEvent(onWsEventStatic);
    server.addHandler(&ws);

    // Catch-all 404 handler
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not Found"); });
  
    server.begin();
    state.startTime = millis();
    Serial.println(F("Web server started."));
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

    StaticJsonDocument<512> doc;
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
        handleWebSocketMessage(client, data, len);
        break;

    default:
        break;
    }
}

// Main WebSocket message handler — now dispatches based on action string
void WebServerManager::handleWebSocketMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len)
{
    if (!modeManager)
    {
        Serial.println(F("[WebServerManager] modeManager is null!"));
        client->text("{\"type\":\"error\",\"message\":\"Mode manager not attached\"}");
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err)
    {
        Serial.println(F("[WebServerManager] Failed to parse JSON"));
        client->text("{\"type\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    JsonVariant json = doc.as<JsonVariant>();

    if (!json.containsKey("action"))
    {
        Serial.println(F("[WebServerManager] Missing 'action' field"));
        client->text("{\"type\":\"error\",\"message\":\"Missing action field\"}");
        return;
    }

    String action = json["action"].as<String>();

    Serial.printf("[WebServerManager] Received action: %s\n", action.c_str());

    // Dispatch based on action string
    for (int i = 0; i < actionMapSize; i++)
    {
        if (action.equalsIgnoreCase(actionMap[i].actionName))
        {
            ActionHandler handler = actionMap[i].handler;
            if (json.containsKey("data"))
                (this->*handler)(client, json["data"]);
        else
            (this->*handler)(client, json);
        String jsonString;
        serializeJson(json, jsonString);
        Serial.printf("[WebServerManager] JSON: %s\n", jsonString.c_str());

        return;
        }
    }

    // Unknown action
    Serial.printf("[WebServerManager] Unknown action: %s\n", action.c_str());
    String errorMsg = "{\"type\":\"error\",\"message\":\"Unknown action: " + action + "\"}";
    client->text(errorMsg);
}

// === Action Handlers ===

// Handles controlUpdate action (mostly state and mode updates)
void WebServerManager::handleControlUpdate(AsyncWebSocketClient *client, JsonVariant data)
{
    if (!data.is<JsonObject>())
    {
        client->text("{\"type\":\"error\",\"message\":\"Missing or invalid data field\"}");
        return;
    }
    JsonObject obj = data.as<JsonObject>();

    // temp_setpoint update
    if (obj.containsKey("temp_setpoint"))
    {
        float newSetpoint = obj["temp_setpoint"].as<float>();
        updateStateProperty(state.tempSetpoint, newSetpoint, "Temperature setpoint");

        if (state.mode == "Hold" && modeManager)
        {
            modeManager->setHold(newSetpoint);
        }
    }

    // rpm_setpoint update
    if (obj.containsKey("rpm_setpoint"))
    {
        int newRpm = obj["rpm_setpoint"].as<int>();
        updateStateProperty(state.rpmSetpoint, newRpm, "RPM setpoint");
    }

    // mode update
    if (obj.containsKey("mode"))
    {
        String newMode = obj["mode"].as<String>();
        if (newMode != state.mode &&
            (newMode == "Hold" || newMode == "Ramp" || newMode == "Timer" || newMode == "Recrystallization" || newMode == "Off"))
        {
            updateMode(newMode);

            if (modeManager)
            {
                if (newMode == "Hold")
                {
                    modeManager->setHold(state.tempSetpoint);
                }
                else if (newMode == "Ramp")
                {
                    float rampRate = 1.0f; // default ramp rate
                    if (obj.containsKey("ramp_rate"))
                    {
                        rampRate = obj["ramp_rate"].as<float>();
                    }
                    modeManager->setRamp(state.tempSetpoint, rampRate, 9999);
                }
                else if (newMode == "Timer")
                {
                    modeManager->setTimer(state.duration, state.tempSetpoint);
                }
                else if (newMode == "Off")
                {
                    modeManager->setOff();
                }
                else
                {
                    Serial.printf("Unknown mode received: %s\n", newMode.c_str());
                }
            }
        }
    }

    // duration update
    if (obj.containsKey("duration"))
    {
        int newDuration = obj["duration"].as<int>();
        if (newDuration != state.duration)
        {
            state.duration = newDuration;
            logEvent("Duration changed to " + String(newDuration));

            if (state.mode == "Timer" && modeManager)
            {
                modeManager->setTimer(state.tempSetpoint, state.duration);
            }
        }
    }

    state.startTime = millis();

    notifyClients();
    client->text("{\"type\":\"ack\",\"message\":\"Update received\"}");
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
    }

    histDoc["type"] = "history";

    String jsonStr;
    serializeJson(histDoc, jsonStr);
    client->text(jsonStr);
}

// Placeholder handlers for Notepad API via WebSocket (if you want to extend)
void WebServerManager::handleNotepadList(AsyncWebSocketClient *client, JsonVariant json)
{
    Serial.println("[WebServerManager] handleNotepadList called");

    StaticJsonDocument<1024> doc;
    doc["type"] = "notepadList"; // <--- Add this line

    JsonArray arr = doc.createNestedArray("experiments");

    Serial.println("[WebServerManager] Populating experiments array...");
    NotepadManager::getInstance().listNotes(arr);

    String out;
    serializeJson(doc, out);

    Serial.println("[WebServerManager] JSON output:");
    Serial.println(out);

    client->text(out);
}

void WebServerManager::handleNotepadLoad(AsyncWebSocketClient *client, JsonVariant data)
{
    if (!data.is<JsonObject>() || !data.containsKey("experiment"))
    {
        client->text("{\"type\":\"error\",\"message\":\"Missing experiment parameter\"}");
        return;
    }
    String experiment = data["experiment"].as<String>();
    String notes;
    if (NotepadManager::getInstance().loadNote(experiment, notes))
    {
        StaticJsonDocument<512> doc;
        doc["type"] = "notepadData";    // <-- Change to notepadData
        doc["experiment"] = experiment; // <-- Add experiment name
        doc["notes"] = notes;
        String json;
        serializeJson(doc, json);
        client->text(json);
    }
    else
    {
        client->text("{\"type\":\"error\",\"message\":\"Note not found\"}");
    }
}

void WebServerManager::handleNotepadSave(AsyncWebSocketClient *client, JsonVariant data)
{
    String jsonStr;
    serializeJson(data, jsonStr);
    printf("[WebServerManager] handleNotepadSave called with data: %s\n", jsonStr.c_str());

       if (!data.is<JsonObject>() || !data.containsKey("experiment") || !data.containsKey("notes"))
    {
        client->text("{\"type\":\"error\",\"message\":\"Missing experiment or notes parameter\"}");
        return;
    }
    String experiment = data["experiment"].as<String>();
    String notes = data["notes"].as<String>();
    if (NotepadManager::getInstance().saveNote(experiment, notes))
    {
        client->text("{\"type\":\"ack\",\"message\":\"Note saved\"}");
    }
    else
    {
        client->text("{\"type\":\"error\",\"message\":\"Failed to save note\"}");
    }
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
        (newMode == "Hold" || newMode == "Ramp" || newMode == "Recrystallization" || newMode == "Timer" || newMode == "Off"))
    {
        logEvent("Mode changed from " + state.mode + " to " + newMode);
        state.mode = newMode;
    }
}

void WebServerManager::logEvent(const String &desc)
{
    if (eventsCount == MAX_EVENTS)
    {
        for (int i = 1; i < MAX_EVENTS; ++i)
            events[i - 1] = events[i];
        eventsCount--;
    }
    events[eventsCount++] = {millis(), desc};
}
