#pragma once
#include <Arduino.h>
#include "HeaterModeManager.h"
#include "WebServerManager.h" // For SystemState

class StateManager {
public:
    static void updateState(float temperature, int rpm, const String &mode, float tempSetpoint, int rpmSetpoint, int duration, HeaterModeManager* modeManager = nullptr);
    static void logState();
};