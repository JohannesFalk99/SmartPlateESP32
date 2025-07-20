/**
 * @file WebServerManager.h
 * @brief Manages the web server and WebSocket communication for the SmartPlateESP32.
 * 
 * This class handles all HTTP and WebSocket communication, including:
 * - Serving static files from LittleFS
 * - Managing WebSocket connections
 * - Processing client commands and state updates
 * - Broadcasting system state to connected clients
 * - Handling file operations via the Notepad API
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <memory>
#include <mutex>
#include "heating/HeaterModeManager.h"
#include "notepad/NotepadManager.h"
#include "config/Config.h"

namespace {
    constexpr size_t HISTORY_SIZE = 300;  ///< Maximum number of history entries to store
    constexpr size_t MAX_EVENTS = 100;    ///< Maximum number of system events to store
    constexpr size_t MAX_WS_MESSAGE_SIZE = 2048;  ///< Maximum WebSocket message size
}

/**
 * @brief Represents a single temperature history entry
 */
struct HistoryEntry {
    unsigned long timestamp;  ///< Unix timestamp of the reading
    float temperature;        ///< Temperature value in Celsius
};

/**
 * @brief Represents a system event log entry
 */
struct EventEntry {
    unsigned long timestamp;  ///< Unix timestamp of the event
    String description;       ///< Event description
};

/**
 * @brief Encapsulates the complete system state
 */
class SystemState {
public:
    // Sensor readings
    float temperature{0.0f};  ///< Current temperature in Celsius
    int rpm{0};               ///< Current RPM of the stirrer

    // Control setpoints
    float tempSetpoint{0.0f};  ///< Target temperature in Celsius
    int rpmSetpoint{0};        ///< Target RPM
    String mode{"Hold"};       ///< Current operation mode
    int duration{0};           ///< Operation duration in seconds

    // Alert thresholds
    float alertTempThreshold{0.0f};  ///< Temperature alert threshold
    float alertRpmThreshold{0.0f};   ///< RPM alert threshold
    int alertTimerThreshold{0};      ///< Timer alert threshold in seconds

    // System info
    unsigned long startTime{0};  ///< System start time in milliseconds

    /**
     * @brief Updates the system state from a JSON object
     * @param json JSON object containing state updates
     * @return true if any field was updated, false otherwise
     */
    bool updateFromJson(const JsonObject& json);

    /**
     * @brief Updates the system state from a JSON object
     * @param json JSON object containing state updates
     * @return true if any field was updated, false otherwise
     */
    bool updateSystemState(const JsonObject& json);
    
    /**
     * @brief Updates the system state using a lambda function
     * @tparam F Callable type that takes a SystemState&
     * @param updater Callable that will update the system state
     */
    template<typename F>
    void updateSystemState(F&& updater) {
        std::lock_guard<std::mutex> lock(mutex_);
        updater(state_);
    }

    /**
     * @brief Serializes the system state to a JSON object
     * @param json JSON object to populate with state
     */
    void toJson(JsonObject& json) const;
};

/**
 * @brief Manages the web server and WebSocket communication
 */
class WebServerManager {
public:
    /**
     * @brief Gets the singleton instance of WebServerManager
     * @return Reference to the WebServerManager instance
     */
    static WebServerManager& instance();

    // Prevent copying and assignment
    WebServerManager(const WebServerManager&) = delete;
    WebServerManager& operator=(const WebServerManager&) = delete;

    /**
     * @brief Gets the underlying AsyncWebServer instance
     * @return Reference to the AsyncWebServer instance
     */
    static AsyncWebServer& server();
    
    /**
     * @brief Initializes the web server and connects to WiFi
     * @param ssid WiFi SSID to connect to
     * @param password WiFi password
     * @return true if initialization was successful, false otherwise
     */
    bool begin(const char* ssid, const char* password);

    /**
     * @brief Processes pending WebSocket events
     * Should be called regularly from the main loop
     */
    void handle();

    /**
     * @brief Notifies all connected clients of the current system state
     */
    void notifyClients();

    /**
     * @brief Attaches the HeaterModeManager for control operations
     * @param manager Pointer to the HeaterModeManager instance
     */
    void attachModeManager(HeaterModeManager* manager);

    /**
     * @brief Updates the system mode in the system state
     * @param mode The new system mode
     */
    void setSystemMode(const String& mode);

    /**
     * @brief Adds a temperature reading to the history
     * @param temperature Temperature in Celsius
     */
    void addHistoryEntry(float temperature);

    /**
     * @brief Updates the system state from a JSON object
     * @param state JSON object containing the state updates
     */
    void updateSystemState(const JsonObject& state) {
        state_.updateSystemState(state);
    }

private:
    WebServerManager() = default;
    ~WebServerManager();

    // WebSocket event handlers
    static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                         AwsEventType type, void* arg, uint8_t* data, size_t len);

    // WebSocket message processing
    void handleWebSocketMessage(AsyncWebSocketClient* client, const uint8_t* data, size_t len);
    void processJsonMessage(AsyncWebSocketClient* client, const JsonDocument& doc);
    void sendError(AsyncWebSocketClient* client, const String& message);

    // Action handler type and mapping
    using ActionHandler = void (WebServerManager::*)(AsyncWebSocketClient*, const JsonVariant&);
    
    struct ActionMapping {
        const char* actionName;
        ActionHandler handler;
    };

    // Action handlers
    void handleControlUpdate(AsyncWebSocketClient* client, const JsonVariant& data);
    void handleGetHistory(AsyncWebSocketClient* client, const JsonVariant& data);
    void handleNotepadList(AsyncWebSocketClient* client, const JsonVariant& data);
    void handleNotepadLoad(AsyncWebSocketClient* client, const JsonVariant& data);
    void handleNotepadSave(AsyncWebSocketClient* client, const JsonVariant& data);

    // Helper methods
    void logEvent(const String& description);
    void updateMode(const String& newMode);
    bool initializeFileSystem();
    void initializeWebServer();
    void initializeWebSocket();
    void cleanupClients();
        
    // State property update utilities
    void updateStateProperty(float& var, float val, const char* name);
    void updateStateProperty(int& var, int val, const char* name);

    // Static server and WebSocket instances
    static std::unique_ptr<AsyncWebServer> server_;
    static std::unique_ptr<AsyncWebSocket> ws_;
    static std::mutex mutex_;
    
    // System state and history
    SystemState state_;
    std::vector<HistoryEntry> history_;
    std::vector<EventEntry> events_;
    
    // Dependencies
    HeaterModeManager* modeManager_{nullptr};
    size_t historyIndex_{0};
    size_t eventsCount_{0};

    // Action handler mapping
    static const std::array<ActionMapping, 5> actionMap_;
};
