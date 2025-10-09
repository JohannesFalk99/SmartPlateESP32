#pragma once

#include <Arduino.h>
#include "HeatingElement.h"

class HeaterModeManager
{
public:
    enum class Mode { OFF, RAMP, HOLD, TIMER };

    HeaterModeManager(HeatingElement &heater);
    void setOff();
    void setRamp(float startTemp, float endTemp, unsigned long durationSeconds);
    void setHold(float holdTemp);
    void setTimer(unsigned long durationSeconds, float targetTemp = 0, bool useTemp = false);
    void update(float currentTemp);
    void setOnCompleteCallback(void (*cb)());
    void setOnFaultCallback(void (*cb)());
    Mode getCurrentMode() const;
    static String modeToString(Mode mode);
    void setMode(const String& modeStr);
    void setMode(Mode modeVal);
    void setTargetTemperature(float temp);
    void setRampParams(float startTemp, float endTemp, unsigned long durationSeconds);
    void setTimerParams(unsigned long durationSeconds, float targetTemp, bool useTemp);
    void setHoldTemp(float holdTemp);
private:
    HeatingElement &heater;
    Mode mode = Mode::OFF;
    float rampStartTemp = 0;
    float rampEndTemp = 0;
    unsigned long rampDuration = 0;
    unsigned long rampStartTime = 0;
    unsigned long timerDuration = 0;
    unsigned long timerStartTime = 0;
    bool timerUseTemp = false;
    void (*onComplete)() = nullptr;
    void (*onFault)() = nullptr;
};
