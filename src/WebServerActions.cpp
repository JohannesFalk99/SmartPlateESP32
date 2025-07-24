#include "WebServerActions.h"
#include "Managers/WebServerManager.h"
#include "Managers/StateManager.h"
#include "Managers/NotepadManager.h"
#include "SerialRemote.h"
namespace WebServerActions {
    void sendAck(AsyncWebSocketClient *client, const String &message)
    {
        logMessagef(LogLevel::INFO, "[WebServerActions] Sending ACK: %s", message.c_str());
        StaticJsonDocument<128> doc;
        doc["type"] = "ack";
        doc["message"] = message;
        String json;
        serializeJson(doc, json);
        client->text(json);
    }

    void sendError(AsyncWebSocketClient *client, const String &error)
    {
        logMessagef(LogLevel::ERROR, "[WebServerActions] Sending ERROR: %s", error.c_str());
        StaticJsonDocument<128> doc;
        doc["type"] = "error";
        doc["message"] = error;
        String json;
        serializeJson(doc, json);
        client->text(json);
    }
    

void handleControlUpdate(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    logMessagef(LogLevel::INFO, "[WebServerActions] handleControlUpdate called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    mgr->handleControlUpdate(client, data);
}
void handleGetHistory(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    logMessagef(LogLevel::INFO, "[WebServerActions] handleGetHistory called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    mgr->handleGetHistory(client, data);
}
void handleNotepadList(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    logMessagef(LogLevel::INFO, "[WebServerActions] handleNotepadList called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    mgr->handleNotepadList(client, data);
}
void handleNotepadLoad(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    logMessagef(LogLevel::INFO, "[WebServerActions] handleNotepadLoad called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    if (!data.isNull())
    {
        logMessagef(LogLevel::INFO, "[WebServerActions] notepadLoad: data field present");
        mgr->handleNotepadLoad(client, data);
    }
    else
    {
        logMessagef(LogLevel::INFO, "[WebServerActions] notepadLoad: data field MISSING");
        sendError(client, "Missing 'data' field for notepadLoad");
    }
}
void handleNotepadSave(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    logMessagef(LogLevel::INFO, "[WebServerActions] handleNotepadSave called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    mgr->handleNotepadSave(client, data);
}


}