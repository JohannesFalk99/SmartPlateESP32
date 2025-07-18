#include <Arduino.h>
#include "WebServerManager.h"
#include "HeatingElement.h"
#include "HeaterModeManager.h"

#define WIFI_SSID "NETGEAR67"
#define WIFI_PASS "IHaveATinyPenis"



#define RELAY_PIN 5
#define MAX_TEMP_LIMIT 70.0f
#define TEMP_FILTER_SIZE 5

HeatingElement heater(RELAY_PIN, MAX_TEMP_LIMIT, TEMP_FILTER_SIZE);

HeaterModeManager modeManager(heater);

int rpm = 100;

unsigned long lastUpdate = 0;

// Forward declarations for callbacks
void handleComplete();
void handleFault();

void setup()
{
    Serial.begin(115200);

    // Setup heater callbacks
    heater.setOnFaultCallback(handleFault);
    modeManager.setOnCompleteCallback(handleComplete);
    modeManager.setOnFaultCallback(handleFault);

    // Initialize heater hardware (e.g., MAX31865 SPI and sensor)
    heater.begin(); // Make sure you implement this to init MAX31865

    // Use singleton properly
    WebServerManager::instance()->attachModeManager(&modeManager);
    WebServerManager::instance()->begin(WIFI_SSID, WIFI_PASS);

    // Initial mode
    modeManager.setHold(30.0);
    state.mode = "Hold";
}

void loop()
{
    WebServerManager::instance()->handle();

    // Update heater and sensor reading every 2 seconds
    if (millis() - lastUpdate > 500)
    {
        lastUpdate = millis();

        // Read sensor and update heater control
        heater.update();

        // Simulate RPM changes (you may replace this with real RPM reading)
        rpm = (rpm >= 200) ? 100 : rpm + 10;

        // Update state with current temperature and rpm
        state.temperature = heater.getCurrentTemperature();
        state.rpm = rpm;

        // Get current mode as string
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

        // Notify connected WebSocket clients
        WebServerManager::instance()->notifyClients();

        Serial.printf("Update sent: Temp=%.2f, RPM=%d, Mode=%s\n",
                      state.temperature, state.rpm, state.mode.c_str());
    }

    // Update heater mode logic based on current temperature
    modeManager.update(heater.getCurrentTemperature());
}

// Callback when mode (Ramp/Timer) completes
void handleComplete()
{
    Serial.println("[HeaterModeManager] Operation complete.");
}

// Callback on fault (e.g., overtemp)
void handleFault()
{
    Serial.println("[HeaterModeManager] FAULT detected! Heater stopped.");
}
