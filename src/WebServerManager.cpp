#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "WebServerManager.h"

#include "NotepadManager.h"

// Define the static server members
AsyncWebServer WebServerManager::server(80);
AsyncWebSocket WebServerManager::ws("/ws");

// Define shared state
SystemState state;
HistoryEntry history[HISTORY_SIZE] = {};
int historyIndex = 0;

EventEntry events[MAX_EVENTS] = {};
int eventsCount = 0;

WebServerManager::WebServerManager() {}

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

    // Notepad REST API
    server.on("/api/notepad", HTTP_GET, [](AsyncWebServerRequest *request) {
        String experiment = "";
        if (request->hasParam("experiment"))
            experiment = request->getParam("experiment")->value();
        String notes;
        if (NotepadManager::getInstance().loadNote(experiment, notes)) {
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            response->print("{\"notes\":");
            response->print('"' + notes + '"');
            response->print("}");
            request->send(response);
        } else {
            request->send(404, "application/json", "{\"error\":\"Not found\"}");
        }
    });
    server.on("/api/notepad", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<1024> doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            String experiment = doc["experiment"] | "";
            String notes = doc["notes"] | "";
            if (NotepadManager::getInstance().saveNote(experiment, notes)) {
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(500, "application/json", "{\"error\":\"Save failed\"}");
            }
        });

    // Setup WebSocket
    ws.onEvent(onWsEventStatic);
    server.addHandler(&ws);

    // Catch-all 404
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

    String json;
    serializeJson(doc, json);
    ws.textAll(json);
}

// WebSocket event handlers
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
        client->text("{\"type\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    JsonVariant json = doc.as<JsonVariant>();

    if (!json.containsKey("action"))
    {
        client->text("{\"type\":\"error\",\"message\":\"Missing action field\"}");
        return;
    }

    String action = json["action"].as<String>();

    if (action == "controlUpdate")
    {
        if (!json.containsKey("data") || !json["data"].is<JsonObject>())
        {
            client->text("{\"type\":\"error\",\"message\":\"Missing or invalid data field\"}");
            return;
        }

        JsonObject data = json["data"].as<JsonObject>();

        // Handle temperature setpoint update
        if (data.containsKey("temp_setpoint"))
        {
            float newSetpoint = data["temp_setpoint"].as<float>();
            updateStateProperty(state.tempSetpoint, newSetpoint, "Temperature setpoint");

            // Apply immediately if mode is Hold
            if (state.mode == "Hold" && modeManager)
            {
                modeManager->setHold(newSetpoint);
            }
        }

        // Handle RPM setpoint update
        if (data.containsKey("rpm_setpoint"))
        {
            int newRpm = data["rpm_setpoint"].as<int>();
            updateStateProperty(state.rpmSetpoint, newRpm, "RPM setpoint");
        }

        // Handle mode update and call corresponding modeManager methods
        if (data.containsKey("mode"))
        {
            String newMode = data["mode"].as<String>();
            if (newMode != state.mode &&
                (newMode == "Hold" || newMode == "Ramp" || newMode == "Timer" || newMode == "Recrystallization"))
            {
                updateMode(newMode);

                if (data.containsKey("mode"))
                {
                    String newMode = data["mode"].as<String>();
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
                            if (data.containsKey("ramp_rate"))
                            {
                                rampRate = data["ramp_rate"].as<float>();
                            }
                            modeManager->setRamp(state.tempSetpoint, rampRate, 9999);
                        }
                        else if (newMode == "Timer")
                        {
                            modeManager->setTimer(state.duration, state.tempSetpoint);
                        }
                        else if (newMode == "Off")
                        {
                            modeManager->setOff(); // Make sure this method exists!
                        }
                        
                        else
                        {
                            // Handle other modes or ignore unknown modes
                            Serial.printf("Unknown mode received: %s\n", newMode.c_str());
                        }
                    }
                }
            }
        }

        // Handle duration update
        if (data.containsKey("duration"))
        {
            int newDuration = data["duration"].as<int>();
            if (newDuration != state.duration)
            {
                state.duration = newDuration;
                logEvent("Duration changed to " + String(newDuration));

                // If current mode is Timer, update timer with new duration
                if (state.mode == "Timer" && modeManager)
                {
                    modeManager->setTimer(state.tempSetpoint, state.duration);
                }
            }
        }

        // Reset start time for timing modes
        state.startTime = millis();

        notifyClients();
        client->text("{\"type\":\"ack\",\"message\":\"Update received\"}");
    }
    else if (action == "getHistory")
    {
        StaticJsonDocument<2048> histDoc;
        JsonArray arr = histDoc.createNestedArray("data");

        for (int i = 0; i < HISTORY_SIZE; i++)
        {
            if (history[i].timestamp == 0)
                continue;

            JsonObject entry = arr.createNestedObject();
            entry["time"] = millis() - history[i].timestamp;
            entry["temperature"] = history[i].temperature;
        }

        histDoc["type"] = "history";

        String jsonStr;
        serializeJson(histDoc, jsonStr);
        client->text(jsonStr);
    }
    else
    {
        client->text("{\"type\":\"error\",\"message\":\"Unknown action\"}");
    }
}

// Utility methods to update state and log events
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
        (newMode == "Hold" || newMode == "Ramp" || newMode == "Recrystallization" || newMode == "Timer"))
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

// Singleton accessor
WebServerManager *WebServerManager::instance()
{
    static WebServerManager inst;
    return &inst;
}
