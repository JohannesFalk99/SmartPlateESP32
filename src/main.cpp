#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "WebServerManager.h"
#include "HeatingElement.h"
#include "HeaterModeManager.h"
#include "Explorer.h"
#include "config/Config.h"

// Hardware Configuration
#define RELAY_PIN 5
#define MAX_TEMP_LIMIT 70.0f
#define TEMP_FILTER_SIZE 5

// System Configuration
#define UPDATE_INTERVAL_MS 500
#define RPM_MIN 100
#define RPM_MAX 200
#define RPM_INCREMENT 10
#define SERIAL_TCP_PORT 23

// System Objects
HeatingElement heater(RELAY_PIN, MAX_TEMP_LIMIT, TEMP_FILTER_SIZE);
HeaterModeManager modeManager(heater);
FileSystemExplorer *explorer = nullptr;

// System State
int rpm = RPM_MIN;
unsigned long lastUpdate = 0;

// TCP Remote Serial Monitor
WiFiServer serialServer(SERIAL_TCP_PORT);
WiFiClient serialClient;

// Forward declarations
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

void temperatureChanged(float newTemp)
{
    Serial.printf("[Temperature] Changed to %.2f°C\n", newTemp);
    WebServerManager::instance()->addHistoryEntry(newTemp);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("[System] Starting SmartPlate ESP32...");

    // Setup heater callbacks
    heater.setOnFaultCallback(handleFault);
    heater.setOnTemperatureChangedCallback(temperatureChanged);
    modeManager.setOnCompleteCallback(handleComplete);
    modeManager.setOnFaultCallback(handleFault);

    // Initialize hardware
    heater.begin();
    
    // Initialize network services
    setupWiFi();
    setupOTA();
    setupWebServer();

    // Initialize system state
    state.mode = Modes::OFF;
    
    Serial.println("[System] Setup complete!");
}

void handleRemoteSerial()
{
    // Accept new TCP clients for remote serial
    if (!serialClient || !serialClient.connected())
    {
        WiFiClient newClient = serialServer.available();
        if (newClient)
        {
            serialClient = newClient;
            Serial.println("[SerialServer] Client connected");
        }
    }

    // Forward Serial data to TCP client
    while (Serial.available())
    {
        char c = Serial.read();
        if (serialClient && serialClient.connected())
        {
            serialClient.write(c);
        }
    }

    // Forward TCP client data back to Serial
    if (serialClient && serialClient.connected() && serialClient.available())
    {
        char c = serialClient.read();
        Serial.write(c);
    }
}

void loop()
{
    // Handle network services
    ArduinoOTA.handle();
    WebServerManager::instance()->handle();
    
    // Handle TCP serial connections
    handleRemoteSerial();

    // Update system state periodically
    if (millis() - lastUpdate > UPDATE_INTERVAL_MS)
    {
        lastUpdate = millis();
        
        heater.update();
        updateRPM();
        updateSystemState();
        
        WebServerManager::instance()->notifyClients();
        
        Serial.printf("[Status] Temp=%.2f°C, RPM=%d, Mode=%s\n",
                      state.temperature, state.rpm, state.mode.c_str());
    }

    modeManager.update(heater.getCurrentTemperature());
}

void handleComplete()
{
    Serial.println("[HeaterModeManager] Operation complete");
}

void handleFault()
{
    Serial.println("[HeaterModeManager] FAULT detected! Heater stopped");
}

void setupWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[WiFi] Connecting");
    
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

void setupOTA()
{
    ArduinoOTA.setHostname("ESP32-SmartPlate");
    
    ArduinoOTA.onStart([]() {
        Serial.println("[OTA] Update started");
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update complete");
    });
    
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

void setupWebServer()
{
    // Start TCP remote serial server
    serialServer.begin();
    serialServer.setNoDelay(true);
    Serial.printf("[SerialServer] Started on port %d\n", SERIAL_TCP_PORT);

    // Setup web server components
    WebServerManager::instance()->attachModeManager(&modeManager);

    // Setup file system explorer
    explorer = new FileSystemExplorer(WebServerManager::instance()->getServer());
    explorer->begin();
    Serial.println("[FileSystem] Explorer initialized");

    // Start the web server
    WebServerManager::instance()->begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("[WebServer] Started");
}

void updateRPM()
{
    rpm = (rpm >= RPM_MAX) ? RPM_MIN : rpm + RPM_INCREMENT;
}

void updateSystemState()
{
    state.temperature = heater.getCurrentTemperature();
    state.rpm = rpm;
    state.mode = getModeString(modeManager.getCurrentMode());
}

const char* getModeString(HeaterModeManager::Mode mode)
{
    switch (mode)
    {
        case HeaterModeManager::OFF: return Modes::OFF;
        case HeaterModeManager::RAMP: return Modes::RAMP;
        case HeaterModeManager::HOLD: return Modes::HOLD;
        case HeaterModeManager::TIMER: return Modes::TIMER;
        default: return Modes::OFF;
    }
}
