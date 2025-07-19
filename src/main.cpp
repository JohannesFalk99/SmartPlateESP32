#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "WebServerManager.h"
#include "HeatingElement.h"
#include "HeaterModeManager.h"
#include "Explorer.h"

#define WIFI_SSID "NETGEAR67"
#define WIFI_PASS "IHaveATinyPenis"

#define RELAY_PIN 5
#define MAX_TEMP_LIMIT 70.0f
#define TEMP_FILTER_SIZE 5

HeatingElement heater(RELAY_PIN, MAX_TEMP_LIMIT, TEMP_FILTER_SIZE);
HeaterModeManager modeManager(heater);
FileSystemExplorer *explorer = nullptr;

int rpm = 100;
unsigned long lastUpdate = 0;

// TCP Remote Serial Monitor
WiFiServer serialServer(23);
WiFiClient serialClient;

// Define global state structure (assuming from your original code)


// Forward declarations for callbacks
void handleComplete();
void handleFault();

void temperatureChanged(float newTemp)
{
    Serial.printf("Callback: Temperature changed to %.2f\n", newTemp);
    WebServerManager::instance()->addHistoryEntry(newTemp);
    // Do something with newTemp here...
}

void setup()
{
    Serial.begin(115200);

    // Setup heater callbacks
    heater.setOnFaultCallback(handleFault);
    heater.setOnTemperatureChangedCallback(temperatureChanged);
    modeManager.setOnCompleteCallback(handleComplete);
    modeManager.setOnFaultCallback(handleFault);

    // Initialize heater hardware (e.g., MAX31865 SPI and sensor)
    heater.begin();

    // Initialize WiFi and OTA before starting the webserver
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected with IP: " + WiFi.localIP().toString());

    ArduinoOTA.setHostname("ESP32-Heater");
    ArduinoOTA.onStart([]()
                       { Serial.println("OTA Start"); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nOTA End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
                           Serial.printf("OTA Error[%u]: ", error);
                           if (error == OTA_AUTH_ERROR)
                               Serial.println("Auth Failed");
                           else if (error == OTA_BEGIN_ERROR)
                               Serial.println("Begin Failed");
                           else if (error == OTA_CONNECT_ERROR)
                               Serial.println("Connect Failed");
                           else if (error == OTA_RECEIVE_ERROR)
                               Serial.println("Receive Failed");
                           else if (error == OTA_END_ERROR)
                               Serial.println("End Failed"); });
    ArduinoOTA.begin();

    // Start TCP remote serial server
    serialServer.begin();
    serialServer.setNoDelay(true);

    // Use singleton WebServerManager properly
    WebServerManager::instance()->attachModeManager(&modeManager);

    // Create FileSystemExplorer using the AsyncWebServer instance from WebServerManager
    explorer = new FileSystemExplorer(WebServerManager::instance()->getServer());
    explorer->begin(); // Setup FS routes

    // Start the WebServerManager after explorer is ready
    WebServerManager::instance()->begin(WIFI_SSID, WIFI_PASS);

    // Initial mode string
    state.mode = "Off";
}

void handleRemoteSerial()
{
    if (!serialClient || !serialClient.connected())
    {
        serialClient = serialServer.available(); // accept new client
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

    // Forward TCP client data back to Serial (optional)
    if (serialClient && serialClient.connected() && serialClient.available())
    {
        char c = serialClient.read();
        Serial.write(c);
    }
}

void loop()
{
    ArduinoOTA.handle();

    WebServerManager::instance()->handle();

    if (!serialClient || !serialClient.connected())
    {
        WiFiClient newClient = serialServer.available();
        if (newClient)
        {
            serialClient = newClient;
            Serial.println("[SerialServer] Client connected");
        }
    }

    handleRemoteSerial();

    if (millis() - lastUpdate > 500)
    {
        lastUpdate = millis();

        heater.update();

        rpm = (rpm >= 200) ? 100 : rpm + 10;

        state.temperature = heater.getCurrentTemperature();
        state.rpm = rpm;

        HeaterModeManager::Mode mode = modeManager.getCurrentMode();
        switch (mode)
        {
        case HeaterModeManager::OFF:
            state.mode = "Off";
            break;
        case HeaterModeManager::RAMP:
            state.mode = "Ramp";
            break;
        case HeaterModeManager::HOLD:
            state.mode = "Hold";
            break;
        case HeaterModeManager::TIMER:
            state.mode = "Timer";
            break;
        }

        WebServerManager::instance()->notifyClients();

        Serial.printf("Update sent: Temp=%.2f, RPM=%d, Mode=%s\n",
                      state.temperature, state.rpm, state.mode.c_str());
    }

    modeManager.update(heater.getCurrentTemperature());
}

void handleComplete()
{
    Serial.println("[HeaterModeManager] Operation complete.");
}

void handleFault()
{
    Serial.println("[HeaterModeManager] FAULT detected! Heater stopped.");
}
