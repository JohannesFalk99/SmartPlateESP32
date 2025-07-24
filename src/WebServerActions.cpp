#include "WebServerActions.h"
#include "Managers/WebServerManager.h"
#include "Managers/StateManager.h"
#include "Managers/NotepadManager.h"

namespace WebServerActions {
    void sendAck(AsyncWebSocketClient *client, const String &message)
    {
        Serial.printf("[WebServerActions] Sending ACK: %s\n", message.c_str());
        StaticJsonDocument<128> doc;
        doc["type"] = "ack";
        doc["message"] = message;
        String json;
        serializeJson(doc, json);
        client->text(json);
    }

    void sendError(AsyncWebSocketClient *client, const String &error)
    {
        Serial.printf("[WebServerActions] Sending ERROR: %s\n", error.c_str());
        StaticJsonDocument<128> doc;
        doc["type"] = "error";
        doc["message"] = error;
        String json;
        serializeJson(doc, json);
        client->text(json);
    }
    

void handleControlUpdate(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    Serial.println("[WebServerActions] handleControlUpdate called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    mgr->handleControlUpdate(client, data);
}
void handleGetHistory(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    Serial.println("[WebServerActions] handleGetHistory called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    mgr->handleGetHistory(client, data);
}
void handleNotepadList(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    Serial.println("[WebServerActions] handleNotepadList called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    mgr->handleNotepadList(client, data);
}
void handleNotepadLoad(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    Serial.println("[WebServerActions] handleNotepadLoad called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    if (!data.isNull())
    {
        Serial.println("[WebServerActions] notepadLoad: data field present");
        mgr->handleNotepadLoad(client, data);
    }
    else
    {
        Serial.println("[WebServerActions] notepadLoad: data field MISSING");
        sendError(client, "Missing 'data' field for notepadLoad");
    }
}
void handleNotepadSave(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data) {
    Serial.println("[WebServerActions] handleNotepadSave called");
    serializeJsonPretty(data, Serial);
    Serial.println();
    mgr->handleNotepadSave(client, data);
}


}