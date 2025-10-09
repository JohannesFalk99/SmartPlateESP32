#pragma once

#include <Arduino.h>
#include "hardware/HeatingElement.h"

/**
 * @brief Manages different operating modes for the heating element
 * 
 * This class provides high-level control modes including OFF, RAMP, HOLD, and TIMER
 * modes with automatic temperature control and timing.
 */
class HeaterModeManager
{
public:
    /**
     * @brief Operating modes for the heater
     */
    enum Mode
    {
        OFF,    ///< Heater is off
        RAMP,   ///< Ramping temperature from start to end over time
        HOLD,   ///< Holding constant temperature
        TIMER   ///< Running for a specific duration
    };

    /**
     * @brief Construct a new Heater Mode Manager
     * @param heater Reference to the HeatingElement to control
     */
    HeaterModeManager(HeatingElement &heater);

    /**
     * @brief Turn off the heater
     */
    void setOff();
    
    /**
     * @brief Set ramp mode - gradually change temperature over time
     * @param startTemp Starting temperature in degrees Celsius
     * @param endTemp Target end temperature in degrees Celsius
     * @param durationSeconds Duration of ramp in seconds
     */
    void setRamp(float startTemp, float endTemp, unsigned long durationSeconds);
    
    /**
     * @brief Set hold mode - maintain constant temperature
     * @param holdTemp Temperature to maintain in degrees Celsius
     */
    void setHold(float holdTemp);
    
    /**
     * @brief Set timer mode - run for specific duration
     * @param durationSeconds Duration to run in seconds
     * @param targetTemp Optional target temperature in degrees Celsius
     * @param useTemp Whether to use temperature control (true) or just timing (false)
     */
    void setTimer(unsigned long durationSeconds, float targetTemp = 0, bool useTemp = false);
    
    /**
     * @brief Update the mode manager state - call regularly in loop
     * @param currentTemp Current temperature reading in degrees Celsius
     */
    void update(float currentTemp);
    
    /**
     * @brief Set callback to be called when operation completes
     * @param cb Function pointer to callback
     */
    void setOnCompleteCallback(void (*cb)());
    
    /**
     * @brief Set callback to be called when fault is detected
     * @param cb Function pointer to callback
     */
    void setOnFaultCallback(void (*cb)());
    
    /**
     * @brief Get the current operating mode
     * @return Mode Current mode enum value
     */
    Mode getCurrentMode() const;
    
    /**
     * @brief Convert Mode enum to human-readable string representation
     * @param mode The Mode enum value to convert
     * @return String representation of the mode ("Off", "Ramp", "Hold", "Timer", or "Unknown")
     */
    static String modeToString(Mode mode);

    /**
     * @brief Set the mode by string or enum
     * @param modeStr The mode as a string ("Off", "Ramp", "Hold", "Timer")
     */
    void setMode(const String& modeStr);
    void setMode(Mode modeVal);

    /**
     * @brief Set the target temperature for the current or next mode
     */
    void setTargetTemperature(float temp);
    /**
     * @brief Set ramp parameters for ramp mode
     */
    void setRampParams(float startTemp, float endTemp, unsigned long durationSeconds);
    /**
     * @brief Set timer parameters for timer mode
     */
    void setTimerParams(unsigned long durationSeconds, float targetTemp, bool useTemp);
    /**
     * @brief Set hold temperature for hold mode
     */
    void setHoldTemp(float holdTemp);

private:
    HeatingElement &heater;
    Mode mode = OFF;

    // Ramp
    float rampStartTemp = 0, rampEndTemp = 0;
    unsigned long rampDuration = 0, rampStartTime = 0;

    // Timer
    unsigned long timerDuration = 0, timerStartTime = 0;
    bool timerUseTemp = false;

    // Callbacks
    void (*onComplete)() = nullptr;
    void (*onFault)() = nullptr;
};
