#pragma once
#include <Arduino.h>
#include "HeaterModeManager.h"
#include "WebServerManager.h" // For SystemState

/**
 * @brief Manages global system state and provides state update utilities
 * 
 * This static class manages the global system state including temperature,
 * RPM, mode, and setpoints, and provides logging functionality.
 */
class StateManager {
public:
    /**
     * @brief Update the global system state
     * @param temperature Current temperature reading in degrees Celsius
     * @param rpm Current RPM value
     * @param mode Current operating mode as string
     * @param tempSetpoint Temperature setpoint in degrees Celsius
     * @param rpmSetpoint RPM setpoint
     * @param duration Duration value in seconds
     * @param modeManager Optional pointer to HeaterModeManager for additional context
     */
    static void updateState(float temperature, int rpm, const String &mode, float tempSetpoint, int rpmSetpoint, int duration, HeaterModeManager* modeManager = nullptr);
    
    /**
     * @brief Log the current system state to serial output
     */
    static void logState();
};