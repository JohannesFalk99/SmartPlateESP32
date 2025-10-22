#pragma once
#include <Arduino.h>
#include "managers/HeaterModeManager.h"
#include "managers/WebServerManager.h" // For SystemState

/**
 * @brief Manages global system state and provides state update utilities
 * 
 * This static class manages the global system state including temperature,
 * RPM, mode, and setpoints, and provides logging functionality.
 * 
 * THREAD SAFETY: All methods accept a mutex parameter for protecting shared state access.
 * Callers must provide the stateMutex to ensure thread-safe operations.
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
     * @param mutex FreeRTOS mutex for protecting shared state (pass nullptr to skip locking)
     * 
     * THREAD SAFETY: Acquires mutex before modifying state, releases after completion.
     * Mutex acquisition timeout is 100ms. State updates are skipped if mutex cannot be acquired.
     */
    static void updateState(float temperature, int rpm, const String &mode, float tempSetpoint, int rpmSetpoint, int duration, HeaterModeManager* modeManager = nullptr, SemaphoreHandle_t mutex = nullptr);
    
    /**
     * @brief Log the current system state to serial output
     * @param mutex FreeRTOS mutex for protecting shared state (pass nullptr to skip locking)
     * 
     * THREAD SAFETY: Acquires mutex before reading state, releases after completion.
     */
    static void logState(SemaphoreHandle_t mutex = nullptr);
};