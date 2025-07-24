#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "Managers/WebServerManager.h"
#include "HeatingElement.h"
#include "Managers/HeaterModeManager.h"
#include "Explorer.h"
#include "config/Config.h"
#include "MAX31865Adapter.h"

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

// --- TCP Remote Serial Monitor ---
WiFiServer serialServer(SERIAL_TCP_PORT);
WiFiClient serialClient;

// --- Logging Utility ---
enum class LogLevel { INFO, ERROR, DEBUG };
void logMessage(LogLevel level, const char* message) {
    const char* levelStr = "";
    switch (level) {
        case LogLevel::INFO: levelStr = "[INFO]"; break;
        case LogLevel::ERROR: levelStr = "[ERROR]"; break;
        case LogLevel::DEBUG: levelStr = "[DEBUG]"; break;
    }
    Serial.printf("%s %s\n", levelStr, message);
}

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

// --- Temperature Change Handler ---
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

void heaterTask(void *pvParameters) {
    while (true) {
        heater.update();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
void webTask(void *pvParameters) {
    while (true) {
        WebServerManager::instance()->handle();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
void stateTask(void *pvParameters) {
    static unsigned long lastUpdate = 0;
    while (true) {
        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100))) {
            if (millis() - lastUpdate > UPDATE_INTERVAL_MS) {
                lastUpdate = millis();
                updateRPM();
                updateSystemState();
                WebServerManager::instance()->notifyClients();
                Serial.printf("[Status] Temp=%.2f°C, RPM=%d, Mode=%s\n",
                              state.temperature, state.rpm, state.mode.c_str());
            }
            modeManager.update(heater.getCurrentTemperature());
            xSemaphoreGive(stateMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// --- Setup Function ---
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
    Serial.println("[System] Setup complete!");
    stateMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(heaterTask, "HeaterTask", 4096, NULL, 1, &heaterTaskHandle, 1);
    xTaskCreatePinnedToCore(webTask, "WebTask", 4096, NULL, 1, &webTaskHandle, 1);
    xTaskCreatePinnedToCore(stateTask, "StateTask", 4096, NULL, 1, &stateTaskHandle, 1);
}

// --- Serial Handling ---
void handleRemoteSerial() {
    if (!serialClient || !serialClient.connected()) {
        WiFiClient newClient = serialServer.available();
        if (newClient) {
            serialClient = newClient;
            Serial.println("[SerialServer] Client connected");
        }
    }
    while (Serial.available()) {
        int c = Serial.read();
        if (serialClient && serialClient.connected() && c != -1) {
            serialClient.write((char)c);
        }
    }
    if (serialClient && serialClient.connected() && serialClient.available()) {
        char c = serialClient.read();
        Serial.write(c);
    }
}

// --- Main Loop ---
void loop() {
    ArduinoOTA.handle();
    handleRemoteSerial();
    vTaskDelay(pdMS_TO_TICKS(10));
}

// --- Callbacks ---
void handleComplete() { logMessage(LogLevel::INFO, "[HeaterModeManager] Operation complete"); }
void handleFault() { logMessage(LogLevel::ERROR, "[HeaterModeManager] FAULT detected! Heater stopped"); }

// --- WiFi Setup ---
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

// --- OTA Setup ---
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

// --- Web Server Setup ---
void setupWebServer() {
    serialServer.begin();
    serialServer.setNoDelay(true);
    Serial.printf("[SerialServer] Started on port %d\n", SERIAL_TCP_PORT);
    WebServerManager::instance()->attachModeManager(&modeManager);
    explorer.begin();
    Serial.println("[FileSystem] Explorer initialized");
    WebServerManager::instance()->begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("[WebServer] Started");
}

// --- State Update Helpers ---
void updateRPM() { rpm = (rpm >= RPM_MAX) ? RPM_MIN : rpm + RPM_INCREMENT; }
void updateState(float temperature, int rpm, HeaterModeManager::Mode mode) {
    state.temperature = temperature;
    state.rpm = rpm;
    state.mode = getModeString(mode);
}
void updateSystemState() { updateState(heater.getCurrentTemperature(), rpm, modeManager.getCurrentMode()); }
const char* getModeString(HeaterModeManager::Mode mode) {
    switch (mode) {
        case HeaterModeManager::OFF: return Modes::OFF;
        case HeaterModeManager::RAMP: return Modes::RAMP;
        case HeaterModeManager::HOLD: return Modes::HOLD;
        case HeaterModeManager::TIMER: return Modes::TIMER;
        default: return Modes::OFF;
    }
}

