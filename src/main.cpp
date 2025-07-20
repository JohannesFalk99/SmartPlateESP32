/**
 * @file main.cpp
 * @brief Main application for SmartPlate ESP32
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <memory>
#include "config/Config.h"
#include "system/WebServerManager.h"
#include "heating/HeatingElement.h"
#include "heating/HeaterModeManager.h"
#include "explorer/Explorer.h"

// Global state
float currentTemperature = 0.0f;
String currentMode = "Off";

// Forward declarations for helper functions
String getModeString();

// Global objects
HeatingElement heater(RELAY_PIN, MAX_TEMP_LIMIT, TEMP_FILTER_SIZE, 17); // CS pin 17 for MAX31865
HeaterModeManager modeManager(heater);
std::unique_ptr<FileSystemExplorer> explorer;
WebServerManager& webServer = WebServerManager::instance();

// Network objects
WiFiServer serialServer(SERIAL_SERVER_PORT);
WiFiClient serialClient;

// Timing variables
unsigned long lastUpdate = 0;

// Forward declarations for callbacks
void handleComplete();
void handleFault();
void temperatureChanged(float newTemp);

// Helper function declarations
bool initializeHardware();
bool initializeWiFi();
void initializeOTA();
void initializeServices();
void handleSerialServer();
bool shouldUpdate();

/**
 * @brief Callback for temperature changes
 * @param newTemp The new temperature reading
 */
void temperatureChanged(float newTemp) {
    DEBUG_PRINTF("Callback: Temperature changed to %.2f°C\n", newTemp);
    // Log temperature change through debug output instead of web server
    DEBUG_PRINTF("Temperature changed to %.2f°C\n", newTemp);
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    DEBUG_PRINTLN("\n=== SmartPlate ESP32 Starting ===");
    
    if (!initializeHardware()) {
        DEBUG_PRINTLN("Hardware initialization failed!");
        return;
    }
    
    if (!initializeWiFi()) {
        DEBUG_PRINTLN("WiFi initialization failed!");
        return;
    }
    
    initializeOTA();
    initializeServices();
    
    DEBUG_PRINTLN("=== Setup Complete ===");
}

// Helper function implementations
bool initializeHardware() {
    DEBUG_PRINTLN("Initializing hardware...");
    
    // Setup heater callbacks
    heater.setOnFaultCallback(handleFault);
    heater.setOnTemperatureChangedCallback(temperatureChanged);
    modeManager.setOnCompleteCallback(handleComplete);
    modeManager.setOnFaultCallback(handleFault);
    
    // Initialize heater hardware (e.g., MAX31865 SPI and sensor)
    if (!heater.begin()) {
        DEBUG_PRINTLN("Failed to initialize heater!");
        return false;
    }
    
    DEBUG_PRINTLN("Hardware initialized successfully");
    return true;
}

bool initializeWiFi() {
    DEBUG_PRINTLN("Initializing WiFi...");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    DEBUG_PRINT("Connecting to WiFi");
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT_MS) {
            DEBUG_PRINTLN("\nWiFi connection timeout!");
            return false;
        }
        delay(WIFI_CONNECT_DELAY_MS);
        DEBUG_PRINT(".");
    }
    
    DEBUG_PRINTLN("\nWiFi connected with IP: " + WiFi.localIP().toString());
    return true;
}

void initializeOTA() {
    DEBUG_PRINTLN("Initializing OTA...");
    
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.onStart([]() {
        DEBUG_PRINTLN("OTA Start");
    });
    ArduinoOTA.onEnd([]() {
        DEBUG_PRINTLN("\nOTA End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        DEBUG_PRINTF("OTA Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        DEBUG_PRINTF("OTA Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR:
                DEBUG_PRINTLN("Auth Failed");
                break;
            case OTA_BEGIN_ERROR:
                DEBUG_PRINTLN("Begin Failed");
                break;
            case OTA_CONNECT_ERROR:
                DEBUG_PRINTLN("Connect Failed");
                break;
            case OTA_RECEIVE_ERROR:
                DEBUG_PRINTLN("Receive Failed");
                break;
            case OTA_END_ERROR:
                DEBUG_PRINTLN("End Failed");
                break;
            default:
                DEBUG_PRINTLN("Unknown Error");
                break;
        }
    });
    ArduinoOTA.begin();
    
    DEBUG_PRINTLN("OTA initialized");
}

/**
 * @brief Initialize all system services
 */
void initializeServices() {
    DEBUG_PRINTLN("Initializing services...");
    
    // Start TCP remote serial server
    serialServer.begin();
    serialServer.setNoDelay(true);
    
    // Initialize web server before explorer to ensure routes are set up correctly
    if (!webServer.begin(WIFI_SSID, WIFI_PASS)) {
        DEBUG_PRINTLN("Failed to start web server!");
        return;
    }
    
    // Create and initialize FileSystemExplorer
    explorer = std::make_unique<FileSystemExplorer>(webServer.server());
    explorer->begin();
    
    DEBUG_PRINTLN("Services initialized");
}

void handleSerialServer() {
    // Check for new client connections
    if (!serialClient || !serialClient.connected()) {
        WiFiClient newClient = serialServer.available();
        if (newClient) {
            serialClient = newClient;
            DEBUG_PRINTLN("[SerialServer] Client connected");
        }
    }
    
    // Forward Serial data to TCP client
    while (Serial.available()) {
        char c = Serial.read();
        if (serialClient && serialClient.connected()) {
            serialClient.write(c);
        }
    }
    
    // Forward TCP client data back to Serial (optional)
    if (serialClient && serialClient.connected() && serialClient.available()) {
        char c = serialClient.read();
        Serial.write(c);
    }
}

bool shouldUpdate() {
    return (millis() - lastUpdate > UPDATE_INTERVAL_MS);
}

/**
 * @brief Update the system state with current values
 */
void updateSystemState() {
    // Update temperature from heater
    currentTemperature = heater.getCurrentTemperature();
    currentMode = getModeString();
    
    // Create a JSON document with the updated state
    StaticJsonDocument<256> doc;
    JsonObject state = doc.to<JsonObject>();
    state["temperature"] = currentTemperature;
    state["mode"] = currentMode;
    state["rpm"] = 100;  // TODO: Get actual RPM from stirrer
    
    // Update the state using the public method
    webServer.updateSystemState(state);
}

/**
 * @brief Get a string representation of the current heater mode
 * @return String representing the current mode
 */
String getModeString() {
    switch (modeManager.getCurrentMode()) {
        case HeaterModeManager::Mode::OFF:    return "Off";
        case HeaterModeManager::Mode::RAMP:   return "Ramp";
        case HeaterModeManager::Mode::HOLD:   return "Hold";
        case HeaterModeManager::Mode::TIMER:  return "Timer";
        default:                              return "Unknown";
    }
}

/**
 * @brief Main application loop
 */
void loop() {
    // Handle OTA updates
    ArduinoOTA.handle();
    
    // Handle WebSocket and HTTP requests
    webServer.handle();
    
    // Handle serial server
    handleSerialServer();
    
    // Update system state periodically
    if (millis() - lastUpdate >= UPDATE_INTERVAL_MS) {
        updateSystemState();
        lastUpdate = millis();
    }
    
    // Update heater control
    modeManager.update(heater.getCurrentTemperature());
    
    // Handle any pending OTA updates
    if (shouldUpdate()) {
        DEBUG_PRINTLN("Restarting for OTA update...");
        ESP.restart();
    }
}

void handleComplete() {
    DEBUG_PRINTLN("[HeaterModeManager] Operation complete.");
}

/**
 * @brief Callback for heater fault conditions
 */
void handleFault() {
    String faultMsg = "Heater fault detected!";
    DEBUG_PRINTLN(faultMsg);
    currentMode = "Fault: " + faultMsg;
    
    // Update system state to reflect fault condition
    webServer.setSystemMode(currentMode);
    
    // Notify all connected clients immediately
    webServer.notifyClients();
}
