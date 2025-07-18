#pragma once

#include <Arduino.h>
#include "HeatingElement.h"

class HeaterModeManager
{
public:
    enum Mode
    {
        OFF,
        RAMP,
        HOLD,
        TIMER
    };

    HeaterModeManager(HeatingElement &heater);

    void setOff();
    void setRamp(float startTemp, float endTemp, unsigned long durationSeconds);
    void setHold(float holdTemp);
    void setTimer(unsigned long durationSeconds, float targetTemp = 0, bool useTemp = false);
    void update(float currentTemp);
    void setOnCompleteCallback(void (*cb)());
    void setOnFaultCallback(void (*cb)());
    Mode getCurrentMode() const;

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
