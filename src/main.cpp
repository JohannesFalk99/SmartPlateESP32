#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "managers/WebServerManager.h"
#include "hardware/HeatingElement.h"
#include "managers/HeaterModeManager.h"
#include "utilities/FileSystemExplorer.h"
#include "config/Config.h"
#include <MAX31865Adapter.h>

#include "utilities/SerialRemote.h"

// Define the chip select pin for MAX31865 (set to your actual GPIO pin)
#define CS_PIN 5

// System Objects
MAX31865Adapter maxSensor(CS_PIN); // Use your actual CS pin
HeatingElement heater(RELAY_PIN, MAX_TEMP_LIMIT, TEMP_FILTER_SIZE, &maxSensor);
HeaterModeManager modeManager(heater);
FileSystemExplorer explorer(WebServerManager::getServer());

// --- System State ---
int rpm = RPM_MIN;
SemaphoreHandle_t stateMutex;

// --- Forward Declarations ---
void setupWiFi();
void setupOTA();
void setupWebServer();
void updateSystemState();
void updateRPM();
const char* getModeString(HeaterModeManager::Mode mode);
void handleRemoteSerial();
void handleComplete();
void handleFault();
void temperatureChanged(float newTemp);

/**
 * @brief Callback handler for temperature changes
 * @param newTemp New temperature value in degrees Celsius
 * 
 * Logs the temperature change and adds entry to history
 */
void temperatureChanged(float newTemp) {
    std::string buffer(50, '\0');
    std::snprintf(buffer.data(), buffer.size(), "[Temperature] Changed to %.2f°C", newTemp);
    logMessage(LogLevel::INFO, buffer.c_str());
    WebServerManager::instance()->addHistoryEntry(newTemp);
}

// --- FreeRTOS Tasks ---
TaskHandle_t heaterTaskHandle = NULL;
TaskHandle_t webTaskHandle = NULL;
TaskHandle_t stateTaskHandle = NULL;

/**
 * @brief FreeRTOS task for heater control
 * @param pvParameters Task parameters (unused)
 * 
 * Updates heater state every 500ms
 */
void heaterTask(void *pvParameters) {
    while (true) {
        heater.update();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief FreeRTOS task for web server handling
 * @param pvParameters Task parameters (unused)
 * 
 * Handles web server events every 50ms
 */
void webTask(void *pvParameters) {
    while (true) {
        WebServerManager::instance()->handle();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief FreeRTOS task for system state management
 * @param pvParameters Task parameters (unused)
 * 
 * Updates system state, RPM, and mode manager periodically
 */
void stateTask(void *pvParameters) {
    static unsigned long lastUpdate = 0;
    while (true) {
        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100))) {
            if (millis() - lastUpdate > UPDATE_INTERVAL_MS) {
                lastUpdate = millis();
                updateRPM();
                updateSystemState();
                WebServerManager::instance()->notifyClients();
                logMessagef(LogLevel::INFO, "[Status] Temp=%.2f°C, RPM=%d, Mode=%s", state.temperature, state.rpm, state.mode.c_str());
            }
            modeManager.update(heater.getCurrentTemperature());
            xSemaphoreGive(stateMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Arduino setup function - initializes system components
 * 
 * Initializes serial, heater, WiFi, OTA, web server, and FreeRTOS tasks
 */
void setup() {
    Serial.begin(115200);
    Serial.println("[System] Starting SmartPlate ESP32...");
    heater.setOnFaultCallback(handleFault);
    heater.setOnTemperatureChangedCallback(temperatureChanged);
    modeManager.setOnCompleteCallback(handleComplete);
    modeManager.setOnFaultCallback(handleFault);
    heater.begin();
    setupWiFi();
    setupOTA();
    setupWebServer();
    state.mode = Modes::OFF;
    logMessagef(LogLevel::INFO, "[System] Setup complete!");
    stateMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(heaterTask, "HeaterTask", 4096, NULL, 1, &heaterTaskHandle, 1);
    xTaskCreatePinnedToCore(webTask, "WebTask", 4096, NULL, 1, &webTaskHandle, 1);
    xTaskCreatePinnedToCore(stateTask, "StateTask", 4096, NULL, 1, &stateTaskHandle, 1);
}

// --- Serial Handling ---
// void handleRemoteSerial() {
//     // Example: Send a periodic message to the TCP client (no USB required)
//     static unsigned long lastMsg = 0;
//     if (millis() - lastMsg > 1000) {
//         Serial.println("[ESP32] Remote TCP debug: SmartPlate is running!");
//         lastMsg = millis();
//     }
//     // If you want to echo data from TCP client to ESP32 log
//     if (Serial.available()) {
//         char c = Serial.read();
//         Serial.write(c); // Optional: echo to hardware Serial
//     }
// }

/**
 * @brief Arduino main loop function
 * 
 * Handles OTA updates and remote serial communication
 */
void loop() {
    ArduinoOTA.handle();
    handleRemoteSerial();
    vTaskDelay(pdMS_TO_TICKS(10));
}

/**
 * @brief Callback handler for operation completion
 */
void handleComplete() { logMessage(LogLevel::INFO, "[HeaterModeManager] Operation complete"); }

/**
 * @brief Callback handler for fault detection
 */
void handleFault() { logMessage(LogLevel::ERROR, "[HeaterModeManager] FAULT detected! Heater stopped"); }

/**
 * @brief Initialize WiFi connection in station mode
 */
void setupWiFi() {
    logMessage(LogLevel::INFO, "[WiFi] Connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        logMessage(LogLevel::DEBUG, "[WiFi] Attempting to connect...");
    }
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "[WiFi] Connected! IP: %s", WiFi.localIP().toString().c_str());
    logMessage(LogLevel::INFO, buffer);
}

/**
 * @brief Initialize Over-The-Air (OTA) update functionality
 * 
 * Sets up OTA with hostname and event callbacks
 */
void setupOTA() {
    ArduinoOTA.setHostname("ESP32-SmartPlate");
    ArduinoOTA.onStart([]() { Serial.println("[OTA] Update started"); });
    ArduinoOTA.onEnd([]() { Serial.println("\n[OTA] Update complete"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR: Serial.println("Auth Failed"); break;
            case OTA_BEGIN_ERROR: Serial.println("Begin Failed"); break;
            case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
            case OTA_END_ERROR: Serial.println("End Failed"); break;
            default: Serial.println("Unknown Error"); break;
        }
    });
    ArduinoOTA.begin();
    Serial.println("[OTA] Ready");
}

/**
 * @brief Initialize web server, file explorer, and remote serial
 */
void setupWebServer() {
    serialServer.begin(23);
    serialServer.setNoDelay(true);
    Serial.printf("[SerialServer] Started on port %d\n", SERIAL_TCP_PORT);
    WebServerManager::instance()->attachModeManager(&modeManager);
    explorer.begin();
    Serial.println("[FileSystem] Explorer initialized");
    WebServerManager::instance()->begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("[WebServer] Started");
}

/**
 * @brief Update RPM value in a cycling pattern
 */
void updateRPM() { rpm = (rpm >= RPM_MAX) ? RPM_MIN : rpm + RPM_INCREMENT; }

/**
 * @brief Update system state with current values
 * @param temperature Current temperature in degrees Celsius
 * @param rpm Current RPM value
 * @param mode Current heater operating mode
 */
void updateState(float temperature, int rpm, HeaterModeManager::Mode mode) {
    state.temperature = temperature;
    state.rpm = rpm;
    state.mode = getModeString(mode);
}

/**
 * @brief Update system state from current sensor readings
 */
void updateSystemState() { updateState(heater.getCurrentTemperature(), rpm, modeManager.getCurrentMode()); }

/**
 * @brief Convert Mode enum to string representation
 * @param mode HeaterModeManager Mode enum value
 * @return const char* String representation of the mode
 */
const char* getModeString(HeaterModeManager::Mode mode) {
    switch (mode) {
        case HeaterModeManager::OFF: return Modes::OFF;
        case HeaterModeManager::RAMP: return Modes::RAMP;
        case HeaterModeManager::HOLD: return Modes::HOLD;
        case HeaterModeManager::TIMER: return Modes::TIMER;
        default: return Modes::OFF;
    }
}

