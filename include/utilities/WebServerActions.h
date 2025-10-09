#pragma once
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

class WebServerManager;

/**
 * @brief Namespace containing WebSocket action handlers
 * 
 * These functions process WebSocket messages for different actions
 * like control updates, history retrieval, and notepad operations.
 */
namespace WebServerActions {
    /**
     * @brief Handle control update messages from client
     * @param mgr Pointer to WebServerManager instance
     * @param client Pointer to WebSocket client
     * @param data JSON data containing control parameters
     */
    void handleControlUpdate(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
    
    /**
     * @brief Handle history data request from client
     * @param mgr Pointer to WebServerManager instance
     * @param client Pointer to WebSocket client
     * @param data JSON data (may be empty)
     */
    void handleGetHistory(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
    
    /**
     * @brief Handle request to list all notepad entries
     * @param mgr Pointer to WebServerManager instance
     * @param client Pointer to WebSocket client
     * @param data JSON data (may be empty)
     */
    void handleNotepadList(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
    
    /**
     * @brief Handle request to load a specific notepad entry
     * @param mgr Pointer to WebServerManager instance
     * @param client Pointer to WebSocket client
     * @param data JSON data containing notepad name to load
     */
    void handleNotepadLoad(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
    
    /**
     * @brief Handle request to save notepad data
     * @param mgr Pointer to WebServerManager instance
     * @param client Pointer to WebSocket client
     * @param data JSON data containing notepad name and content
     */
    void handleNotepadSave(WebServerManager* mgr, AsyncWebSocketClient* client, JsonVariant data);
}