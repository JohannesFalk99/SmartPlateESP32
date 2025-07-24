#pragma once
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

class WebServerManager;

namespace WebServerActions {
    void handleControlUpdate(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
    void handleGetHistory(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
    void handleNotepadList(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
    void handleNotepadLoad(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
    void handleNotepadSave(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
}