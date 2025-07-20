/**
 * @file WebServerManager.cpp
 * @brief Implementation of the WebServerManager class
 */

#include "system/WebServerManager.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "config/Config.h"

// Initialize static members
std::unique_ptr<AsyncWebServer> WebServerManager::server_ = nullptr;
std::unique_ptr<AsyncWebSocket> WebServerManager::ws_ = nullptr;
std::mutex WebServerManager::mutex_;

// Action handler mapping
const std::array<WebServerManager::ActionMapping, 5> WebServerManager::actionMap_ = {{
    {"controlUpdate", &WebServerManager::handleControlUpdate},
    {"getHistory", &WebServerManager::handleGetHistory},
    {"notepadList", &WebServerManager::handleNotepadList},
    {"notepadLoad", &WebServerManager::handleNotepadLoad},
    {"notepadSave", &WebServerManager::handleNotepadSave}
}};

WebServerManager::~WebServerManager() {
    if (ws_) {
        ws_->closeAll();
        ws_.reset();
    }
    if (server_) {
        server_->end();
        server_.reset();
    }
}

WebServerManager& WebServerManager::instance() {
    static WebServerManager instance;
    return instance;
}

AsyncWebServer& WebServerManager::server() {
    if (!server_) {
        server_ = std::make_unique<AsyncWebServer>(80);
    }
    return *server_;
}

bool WebServerManager::begin(const char* ssid, const char* password) {
    DEBUG_PRINTLN(F("[WebServer] Initializing WiFi..."));
    
    // Configure WiFi
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);

    // Wait for connection with timeout
    const unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT_MS) {
            DEBUG_PRINTLN(F("[WebServer] WiFi connection timed out"));
            return false;
        }
        delay(500);
        DEBUG_PRINT(F("."));
    }
    
    DEBUG_PRINTLN();
    DEBUG_PRINTF("[WebServer] Connected to WiFi. IP: %s\n", WiFi.localIP().toString().c_str());

    // Initialize components
    if (!initializeFileSystem()) {
        return false;
    }
    
    initializeWebServer();
    initializeWebSocket();
    
    // Start server
    server().begin();
    state_.startTime = millis();
    
    DEBUG_PRINTLN(F("[WebServer] Web server started"));
    return true;
}

void WebServerManager::handle() {
    if (ws_) {
        ws_->cleanupClients();
    }
    
    // Periodically notify clients of state changes
    static unsigned long lastNotifyTime = 0;
    if (millis() - lastNotifyTime > 1000) {  // Notify every second
        notifyClients();
        lastNotifyTime = millis();
    }
}

void WebServerManager::attachModeManager(HeaterModeManager* manager) {
    std::lock_guard<std::mutex> lock(mutex_);
    modeManager_ = manager;
    DEBUG_PRINTLN(F("[WebServer] HeaterModeManager attached"));
}

void WebServerManager::setSystemMode(const String& mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.mode = mode;
    DEBUG_PRINTF("System mode updated to: %s\n", mode.c_str());
}

void WebServerManager::addHistoryEntry(float temperature) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Add new entry to circular buffer
    history_[historyIndex_ % HISTORY_SIZE] = {millis(), temperature};
    historyIndex_++;
    
    // Notify clients of the update
    notifyClients();
}

bool WebServerManager::updateSystemState(const JsonObject& json) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool updated = state_.updateFromJson(json);
    if (updated) {
        notifyClients();
    }
    return updated;
}

void WebServerManager::notifyClients() {
    if (!ws_ || ws_->count() == 0) {
        return;
    }

    // Create a JSON document with the current state
    DynamicJsonDocument doc(1024);
    doc["type"] = "dataUpdate";
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.toJson(doc.createNestedObject("data"));
    }

    // Serialize and send to all clients
    String json;
    serializeJson(doc, json);
    ws_->textAll(json);
}

// ===== Private Methods =====

bool WebServerManager::initializeFileSystem() {
    if (!LittleFS.begin(true)) {
        DEBUG_PRINTLN(F("[WebServer] Failed to mount LittleFS"));
        return false;
    }
    
    // Check if index.html exists
    if (!LittleFS.exists("/index.html")) {
        DEBUG_PRINTLN(F("[WebServer] index.html not found in LittleFS"));
        return false;
    }
    
    return true;
}

void WebServerManager::initializeWebServer() {
    // Serve static files from LittleFS
    server().serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // Catch-all handler for 404
    server().onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not Found");
    });
    
    // Health check endpoint
    server().on("/health", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
}

void WebServerManager::initializeWebSocket() {
    ws_ = std::make_unique<AsyncWebSocket>("/ws");
    ws_->onEvent(onWsEvent);
    server().addHandler(ws_.get());
}

// WebSocket event handler
void WebServerManager::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT: {
            DEBUG_PRINTF("[WebSocket] Client #%u connected from %s\n", 
                        client->id(), client->remoteIP().toString().c_str());
            // Send current state to newly connected client
            instance().notifyClients();
            break;
        }
        
        case WS_EVT_DISCONNECT:
            DEBUG_PRINTF("[WebSocket] Client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA: {
            // Handle WebSocket message
            AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0;  // Null-terminate the string
                instance().handleWebSocketMessage(client, data, len);
            }
            break;
        }
            
        case WS_EVT_ERROR:
            DEBUG_PRINTF("[WebSocket] Error: %s\n", reinterpret_cast<char*>(arg));
            break;
            
        case WS_EVT_PONG:
            DEBUG_PRINTF("[WebSocket] Received pong from client #%u\n", client->id());
            break;
    }
}

void WebServerManager::handleWebSocketMessage(AsyncWebSocketClient* client, const uint8_t* data, size_t len) {
    // Check message size
    if (len > MAX_WS_MESSAGE_SIZE) {
        sendError(client, "Message too large");
        return;
    }
    
    // Parse JSON
    DynamicJsonDocument doc(MAX_WS_MESSAGE_SIZE);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        DEBUG_PRINTF("[WebSocket] JSON parse error: %s\n", error.c_str());
        sendError(client, "Invalid JSON");
        return;
    }
    
    // Process the message
    processJsonMessage(client, doc);
}

void WebServerManager::processJsonMessage(AsyncWebSocketClient* client, const JsonDocument& doc) {
    if (!doc.containsKey("action")) {
        sendError(client, "Missing 'action' field");
        return;
    }
    
    const char* action = doc["action"];
    const JsonVariant data = doc.containsKey("data") ? doc["data"] : doc;
    
    DEBUG_PRINTF("[WebSocket] Processing action: %s\n", action);
    
    // Find and call the appropriate handler
    for (const auto& mapping : actionMap_) {
        if (strcmp(mapping.actionName, action) == 0) {
            (this->*mapping.handler)(client, data);
            return;
        }
    }
    
    // No handler found for this action
    DEBUG_PRINTF("[WebSocket] Unknown action: %s\n", action);
    sendError(client, "Unknown action");
}

void WebServerManager::sendError(AsyncWebSocketClient* client, const String& message) {
    if (!client || !client->canSend()) {
        return;
    }
    
    DynamicJsonDocument doc(256);
    doc["type"] = "error";
    doc["message"] = message;
    
    String json;
    serializeJson(doc, json);
    client->text(json);
}

// ===== SystemState Methods =====

bool SystemState::updateFromJson(const JsonObject& json) {
    bool updated = false;
    
    if (json.containsKey("temperature")) {
        temperature = json["temperature"];
        updated = true;
    }
    if (json.containsKey("rpm")) {
        rpm = json["rpm"];
        updated = true;
    }
    if (json.containsKey("tempSetpoint")) {
        tempSetpoint = json["tempSetpoint"];
        updated = true;
    }
    if (json.containsKey("rpmSetpoint")) {
        rpmSetpoint = json["rpmSetpoint"];
        updated = true;
    }
    if (json.containsKey("mode")) {
        mode = json["mode"].as<String>();
        updated = true;
    }
    if (json.containsKey("duration")) {
        duration = json["duration"];
        updated = true;
    }
    
    return updated;
}

void SystemState::toJson(JsonObject& json) const {
    json["temperature"] = temperature;
    json["rpm"] = rpm;
    json["tempSetpoint"] = tempSetpoint;
    json["rpmSetpoint"] = rpmSetpoint;
    json["mode"] = mode;
    json["duration"] = duration;
    json["alertTempThreshold"] = alertTempThreshold;
    json["alertRpmThreshold"] = alertRpmThreshold;
    json["alertTimerThreshold"] = alertTimerThreshold;
    json["runningTime"] = (millis() - startTime) / 1000;
}

// ===== Action Handlers =====

void WebServerManager::handleControlUpdate(AsyncWebSocketClient* client, const JsonVariant& data) {
    if (!data.is<JsonObject>()) {
        sendError(client, "Invalid control data");
        return;
    }
    
    bool stateUpdated = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stateUpdated = state_.updateFromJson(data.as<JsonObject>());
    }
    
    if (stateUpdated) {
        // Notify all clients of the state change
        notifyClients();
        
        // Acknowledge the update
        DynamicJsonDocument doc(128);
        doc["type"] = "ack";
        doc["message"] = "Control updated";
        
        String json;
        serializeJson(doc, json);
        client->text(json);
    }
}

void WebServerManager::handleGetHistory(AsyncWebSocketClient* client, const JsonVariant& data) {
    // Get the number of points to return (default to 100)
    size_t count = data.is<JsonObject>() && data.as<JsonObject>().containsKey("count") 
        ? data["count"].as<size_t>() 
        : 100;
        
    count = std::min(count, static_cast<size_t>(HISTORY_SIZE));
    
    DynamicJsonDocument doc(4096);  // Adjust size based on expected history size
    doc["type"] = "historyData";
    JsonArray historyData = doc.createNestedArray("data");
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t startIdx = (historyIndex_ > count) ? (historyIndex_ - count) % HISTORY_SIZE : 0;
        size_t endIdx = std::min(startIdx + count, static_cast<size_t>(HISTORY_SIZE));
        
        for (size_t i = startIdx; i < endIdx; i++) {
            if (i < history_.size()) {
                JsonObject entry = historyData.createNestedObject();
                entry["timestamp"] = history_[i].timestamp;
                entry["temperature"] = history_[i].temperature;
            }
        }
    }
    
    String json;
    serializeJson(doc, json);
    client->text(json);
}

void WebServerManager::handleNotepadList(AsyncWebSocketClient* client, const JsonVariant& data) {
    // Implementation for listing notepad files
    DynamicJsonDocument doc(1024);
    doc["type"] = "notepadList";
    JsonArray files = doc.createNestedArray("files");
    
    // Add example files (replace with actual file system listing)
    files.add("note1.txt");
    files.add("note2.txt");
    
    String json;
    serializeJson(doc, json);
    client->text(json);
}

void WebServerManager::handleNotepadLoad(AsyncWebSocketClient* client, const JsonVariant& data) {
    if (!data.is<JsonObject>() || !data.as<JsonObject>().containsKey("filename")) {
        sendError(client, "Missing filename");
        return;
    }
    
    String filename = data["filename"].as<String>();
    
    // Implementation for loading a notepad file
    File file = LittleFS.open(("/" + filename).c_str(), "r");
    if (!file) {
        sendError(client, "Failed to open file");
        return;
    }
    
    String content = file.readString();
    file.close();
    
    DynamicJsonDocument doc(4096);
    doc["type"] = "notepadContent";
    doc["filename"] = filename;
    doc["content"] = content;
    
    String json;
    serializeJson(doc, json);
    client->text(json);
}

void WebServerManager::handleNotepadSave(AsyncWebSocketClient* client, const JsonVariant& data) {
    if (!data.is<JsonObject>() || 
        !data.as<JsonObject>().containsKey("filename") || 
        !data.as<JsonObject>().containsKey("content")) {
        sendError(client, "Missing filename or content");
        return;
    }
    
    String filename = data["filename"].as<String>();
    String content = data["content"].as<String>();
    
    // Implementation for saving a notepad file
    File file = LittleFS.open(("/" + filename).c_str(), "w");
    if (!file) {
        sendError(client, "Failed to create file");
        return;
    }
    
    size_t bytesWritten = file.print(content);
    file.close();
    
    DynamicJsonDocument doc(256);
    if (bytesWritten > 0) {
        doc["type"] = "ack";
        doc["message"] = "File saved successfully";
    } else {
        doc["type"] = "error";
        doc["message"] = "Failed to write file";
    }
    
    String json;
    serializeJson(doc, json);
    client->text(json);
}

void WebServerManager::logEvent(const String& description) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (events_.size() >= MAX_EVENTS) {
        // Remove the oldest event if we've reached the maximum
        events_.erase(events_.begin());
    }
    
    // Add the new event
    events_.push_back({millis(), description});
    eventsCount_++;
    
    DEBUG_PRINTF("[Event] %s\n", description.c_str());
}

void WebServerManager::updateMode(const String& newMode) {
    if (!modeManager_) {
        DEBUG_PRINTLN(F("[WebServer] Cannot update mode: No mode manager attached"));
        return;
    }
    
    // Update the mode in the state
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.mode = newMode;
    }
    
    // Notify clients of the mode change
    notifyClients();
    
    // Log the mode change
    logEvent("Mode changed to: " + newMode);
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
