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
#include "managers/HeaterModeManager.h"
#include "managers/NotepadManager.h"
#include "config/Config.h"

// Constants
#define HISTORY_SIZE 300
#define MAX_EVENTS 100

// Data structures

/**
 * @brief Structure to store temperature history entries
 */
struct HistoryEntry
{
    unsigned long timestamp;  ///< Timestamp of the reading
    float temperature;        ///< Temperature value in degrees Celsius
};

/**
 * @brief Structure to maintain global system state
 */
struct SystemState
{
    float temperature = 0.0;    ///< Current temperature reading
    int rpm = 0;                ///< Current RPM value

    float tempSetpoint = 0.0;   ///< Temperature setpoint
    int rpmSetpoint = 0;        ///< RPM setpoint
    String mode = Modes::HOLD;  ///< Current operating mode

    int duration = 0;           ///< Duration setting in seconds

    float alertTempThreshold = ALERT_TEMP_THRESHOLD;  ///< Alert threshold for temperature
    float alertRpmThreshold = ALERT_RPM_THRESHOLD;    ///< Alert threshold for RPM
    int alertTimerThreshold = ALERT_TIMER_THRESHOLD;  ///< Alert threshold for timer

    unsigned long startTime = 0;  ///< System start time
};

/**
 * @brief Structure to store system events
 */
struct EventEntry
{
    unsigned long timestamp;  ///< Timestamp of the event
    String description;       ///< Event description text
};

/**
 * @brief Manages web server, WebSocket connections, and system state
 * 
 * This singleton class handles HTTP requests, WebSocket communication,
 * maintains system state and history, and coordinates with the mode manager.
 */
class WebServerManager
{
public:
    /**
     * @brief Construct a new Web Server Manager
     */
    WebServerManager();

    /**
     * @brief Get the singleton instance
     * @return WebServerManager* Pointer to the singleton instance
     */
    static WebServerManager *instance();

    /**
     * @brief Get reference to the web server
     * @return AsyncWebServer& Reference to the AsyncWebServer instance
     */
    static AsyncWebServer &getServer();
    
    /**
     * @brief Initialize WiFi and web server
     * @param ssid WiFi network SSID
     * @param password WiFi network password
     */
    void begin(const char *ssid, const char *password);
    
    /**
     * @brief Handle periodic tasks (call in main loop)
     */
    void handle();
    
    /**
     * @brief Notify all connected WebSocket clients of state changes
     */
    void notifyClients();

    /**
     * @brief Attach a HeaterModeManager for coordinated control
     * @param manager Pointer to HeaterModeManager instance
     */
    void attachModeManager(HeaterModeManager *manager);
    
    /**
     * @brief Add a temperature reading to history
     * @param temperature Temperature value in degrees Celsius
     */
    void addHistoryEntry(float temperature);
    
    /**
     * @brief Handle control update WebSocket message
     * @param client Pointer to WebSocket client
     * @param data JSON data with control parameters
     */
    void handleControlUpdate(AsyncWebSocketClient *client, JsonVariant data);

    /**
     * @brief Handle history retrieval WebSocket message
     * @param client Pointer to WebSocket client
     * @param data JSON data (may be empty)
     */
    void handleGetHistory(AsyncWebSocketClient *client, JsonVariant data);
    
    /**
     * @brief Handle notepad list WebSocket message
     * @param client Pointer to WebSocket client
     * @param data JSON data (may be empty)
     */
    void handleNotepadList(AsyncWebSocketClient *client, JsonVariant data);
    
    /**
     * @brief Handle notepad load WebSocket message
     * @param client Pointer to WebSocket client
     * @param data JSON data with notepad name
     */
    void handleNotepadLoad(AsyncWebSocketClient *client, JsonVariant data);
    
    /**
     * @brief Handle notepad save WebSocket message
     * @param client Pointer to WebSocket client
     * @param data JSON data with notepad name and content
     */
    void handleNotepadSave(AsyncWebSocketClient *client, JsonVariant data);

private:
    // Web server and websocket instances
    static AsyncWebServer server;
    static AsyncWebSocket ws;

    HeaterModeManager *modeManager = nullptr;

    /**
     * @brief Send acknowledgment message to WebSocket client
     * @param client Pointer to WebSocket client
     * @param message Acknowledgment message text
     */
    void sendAck(AsyncWebSocketClient *client, const String &message);
    
    /**
     * @brief Send error message to WebSocket client
     * @param client Pointer to WebSocket client
     * @param error Error message text
     */
    void sendError(AsyncWebSocketClient *client, const String &error);

    /**
     * @brief Send JSON response to WebSocket client
     * @param client Pointer to WebSocket client
     * @param response JSON object to send
     */
    void sendJsonResponse(AsyncWebSocketClient *client, const JsonObject &response);

    /**
     * @brief Initialize WiFi connection
     * @param ssid WiFi network SSID
     * @param password WiFi network password
     * @return true if WiFi connected successfully
     * @return false if connection failed
     */
    bool beginWiFi(const char *ssid, const char *password);
    
    /**
     * @brief Initialize LittleFS file system
     * @return true if file system mounted successfully
     * @return false if mount failed
     */
    bool beginFileSystem();
    
    /**
     * @brief Initialize and start the web server
     */
    void beginServer();

    /**
     * @brief Static WebSocket event handler (dispatches to instance method)
     * @param server Pointer to AsyncWebSocket server
     * @param client Pointer to WebSocket client
     * @param type Event type
     * @param arg Event argument
     * @param data Event data buffer
     * @param len Length of data buffer
     */
    static void onWsEventStatic(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                AwsEventType type, void *arg, uint8_t *data, size_t len);
    
    /**
     * @brief Instance WebSocket event handler
     * @param server Pointer to AsyncWebSocket server
     * @param client Pointer to WebSocket client
     * @param type Event type
     * @param arg Event argument
     * @param data Event data buffer
     * @param len Length of data buffer
     */
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len);

    /**
     * @brief Parse and dispatch WebSocket messages to appropriate handlers
     * @param client Pointer to WebSocket client
     * @param data Message data buffer
     * @param len Length of message data
     */
    void handleWebSocketMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len);

    // === Action handler dispatch system ===
    using ActionHandler = std::function<void(WebServerManager*, AsyncWebSocketClient*, JsonVariant)>;

    /**
     * @brief Structure mapping action strings to handler functions
     */
    struct ActionMapping {
        const char* action;
        ActionHandler handler;
    };

    static const ActionMapping actionMap[];
    static const int actionMapSize;

   

    // Mode handler function type
    using ModeHandler = std::function<void(const JsonObject&)>;
    
    /**
     * @brief Handle OFF mode request
     * @param params JSON object with mode parameters
     */
    void handleModeOff(const JsonObject& params);
    
    /**
     * @brief Handle HOLD mode request
     * @param params JSON object with hold temperature
     */
    void handleModeHold(const JsonObject& params);
    
    /**
     * @brief Handle RAMP mode request
     * @param params JSON object with ramp parameters
     */
    void handleModeRamp(const JsonObject& params);
    
    /**
     * @brief Handle TIMER mode request
     * @param params JSON object with timer parameters
     */
    void handleModeTimer(const JsonObject& params);
    
    /**
     * @brief Initialize mode handler map
     */
    void initializeModeHandlers();
    
    /**
     * @brief Update a float state property and log if changed
     * @param var Reference to state variable
     * @param val New value
     * @param name Property name for logging
     */
    void updateStateProperty(float &var, float val, const char *name);
    
    /**
     * @brief Update an integer state property and log if changed
     * @param var Reference to state variable
     * @param val New value
     * @param name Property name for logging
     */
    void updateStateProperty(int &var, int val, const char *name);
    
    /**
     * @brief Update the system mode and log the change
     * @param newMode New mode string
     */
    void updateMode(const String &newMode);
    
    /**
     * @brief Log a system event
     * @param desc Event description text
     */
    void logEvent(const String &desc);
    
    /**
     * @brief Hash functor for C-string keys in unordered_map
     */
    struct CStringHash {
        std::size_t operator()(const char* str) const {
            std::size_t hash = 0;
            while (*str) {
                hash = hash * 101 + *str++;
            }
            return hash;
        }
    };

    /**
     * @brief Equality functor for C-string keys in unordered_map
     */
    struct CStringEqual {
        bool operator()(const char* a, const char* b) const {
            return strcmp(a, b) == 0;
        }
    };

    std::unordered_map<const char*, ModeHandler, CStringHash, CStringEqual> modeHandlers;
};

/// Global shared system state
extern SystemState state;

/// Current index in the history circular buffer
extern int historyIndex;

/// Current count of logged events
extern int eventsCount;

#endif // WEBSERVERMANAGER_H
