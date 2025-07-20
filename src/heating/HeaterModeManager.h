/**
 * @file HeaterModeManager.h
 * @brief Advanced heating control modes for temperature management
 * @author SmartPlate Project
 * @version 2.0
 */

#pragma once

#include <Arduino.h>
#include <functional>
#include "heating/HeatingElement.h"

/**
 * @class HeaterModeManager
 * @brief Manages different heating modes including ramp, hold, and timer operations
 * 
 * This class provides high-level control modes for the heating element,
 * including temperature ramping, holding at specific temperatures, and
 * timer-based operations with optional temperature targets.
 */
class HeaterModeManager {
public:
    /**
     * @enum Mode
     * @brief Available heating control modes
     */
    enum class Mode : uint8_t {
        OFF,    ///< Heater is off
        RAMP,   ///< Temperature ramping mode
        HOLD,   ///< Temperature holding mode
        TIMER   ///< Timer-based mode
    };
    
    // Type aliases for better code readability
    using Callback = std::function<void()>;
    
    // Configuration constants
    static constexpr float DEFAULT_TOLERANCE = 0.5f;
    static constexpr float HOLD_HYSTERESIS = 1.0f;
    
    /**
     * @brief Constructor
     * @param heater Reference to the heating element to control
     */
    explicit HeaterModeManager(HeatingElement& heater);
    
    // Disable copy constructor and assignment operator
    HeaterModeManager(const HeaterModeManager&) = delete;
    HeaterModeManager& operator=(const HeaterModeManager&) = delete;
    
    /**
     * @brief Turn off the heater and set mode to OFF
     */
    void setOff();
    
    /**
     * @brief Set temperature ramp mode
     * @param startTemp Starting temperature in Celsius
     * @param endTemp Target end temperature in Celsius
     * @param durationSeconds Duration of ramp in seconds
     * @param tolerance Temperature tolerance for control (default: DEFAULT_TOLERANCE)
     */
    void setRamp(float startTemp, float endTemp, unsigned long durationSeconds, 
                 float tolerance = DEFAULT_TOLERANCE);
    
    /**
     * @brief Set temperature hold mode
     * @param holdTemp Temperature to maintain in Celsius
     * @param tolerance Temperature tolerance for control (default: DEFAULT_TOLERANCE)
     */
    void setHold(float holdTemp, float tolerance = DEFAULT_TOLERANCE);
    
    /**
     * @brief Set timer mode
     * @param durationSeconds Timer duration in seconds
     * @param targetTemp Optional target temperature (default: 0)
     * @param useTemp Whether to use temperature control (default: false)
     * @param tolerance Temperature tolerance if using temp control (default: DEFAULT_TOLERANCE)
     */
    void setTimer(unsigned long durationSeconds, float targetTemp = 0.0f, 
                  bool useTemp = false, float tolerance = DEFAULT_TOLERANCE);
    
    /**
     * @brief Update the mode manager (call frequently)
     * @param currentTemp Current temperature reading
     */
    void update(float currentTemp);
    
    /**
     * @brief Check if current operation is complete
     * @return true if operation completed, false otherwise
     */
    bool isComplete() const;
    
    /**
     * @brief Get progress of current operation (0.0 to 1.0)
     * @return Progress percentage as float
     */
    float getProgress() const;
    
    /**
     * @brief Get remaining time for current operation
     * @return Remaining time in milliseconds, 0 if not applicable
     */
    unsigned long getRemainingTime() const;
    
    // Getters
    Mode getCurrentMode() const { return mode; }
    float getCurrentTarget() const;
    bool isActive() const { return mode != Mode::OFF; }
    
    // Callback setters using std::function for better flexibility
    void setOnCompleteCallback(const Callback& cb) { onComplete = cb; }
    void setOnFaultCallback(const Callback& cb) { onFault = cb; }
    
private:
    /**
     * @brief Update ramp mode logic
     */
    void updateRampMode();
    
    /**
     * @brief Update hold mode logic
     * @param currentTemp Current temperature reading
     */
    void updateHoldMode(float currentTemp);
    
    /**
     * @brief Update timer mode logic
     */
    void updateTimerMode();
    
    /**
     * @brief Handle fault condition
     */
    void handleFault();
    
    /**
     * @brief Calculate current target temperature for ramp mode
     * @return Current target temperature
     */
    float calculateRampTarget() const;
    
    // Hardware reference
    HeatingElement& heater;
    
    // Current mode
    Mode mode = Mode::OFF;
    
    // Ramp mode parameters
    struct RampParams {
        float startTemp = 0.0f;
        float endTemp = 0.0f;
        unsigned long duration = 0;
        unsigned long startTime = 0;
        float tolerance = DEFAULT_TOLERANCE;
    } rampParams;
    
    // Hold mode parameters
    struct HoldParams {
        float targetTemp = 0.0f;
        float tolerance = DEFAULT_TOLERANCE;
    } holdParams;
    
    // Timer mode parameters
    struct TimerParams {
        unsigned long duration = 0;
        unsigned long startTime = 0;
        float targetTemp = 0.0f;
        bool useTemp = false;
        float tolerance = DEFAULT_TOLERANCE;
    } timerParams;
    
    // Callbacks
    Callback onComplete;
    Callback onFault;
};
