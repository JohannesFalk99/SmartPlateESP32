#include "services/HeaterModeManager.h"
#include <algorithm>

HeaterModeManager::HeaterModeManager(HeatingElement &heater) : heater(heater) {}

void HeaterModeManager::setOff() {
    mode = Mode::OFF;
    heater.stop();
}

void HeaterModeManager::setRamp(float startTemp, float endTemp, unsigned long durationSeconds) {
    rampStartTemp = startTemp;
    rampEndTemp = endTemp;
    rampDuration = durationSeconds * 1000UL;
    rampStartTime = millis();
    mode = Mode::RAMP;
    heater.setTargetTemperature(startTemp, 0.5f);
}

void HeaterModeManager::setHold(float holdTemp) {
    heater.setTargetTemperature(holdTemp, 0.5f);
    mode = Mode::HOLD;
}

void HeaterModeManager::setTimer(unsigned long durationSeconds, float targetTemp, bool useTemp) {
    timerDuration = durationSeconds * 1000UL;
    timerStartTime = millis();
    timerUseTemp = useTemp;
    if (useTemp) heater.setTargetTemperature(targetTemp, 0.5f);
    mode = Mode::TIMER;
}

void HeaterModeManager::update(float currentTemp) {
    switch (mode) {
        case Mode::OFF:
            break;
        case Mode::RAMP: {
            unsigned long elapsed = millis() - rampStartTime;
            if (elapsed >= rampDuration) {
                heater.setTargetTemperature(rampEndTemp, 0.5f);
                mode = Mode::HOLD; // fall into hold at final temp
                if (onComplete) onComplete();
                break;
            }
            float progress = static_cast<float>(elapsed) / static_cast<float>(rampDuration);
            float target = rampStartTemp + (rampEndTemp - rampStartTemp) * progress;
            heater.setTargetTemperature(target, 0.5f);
            break;
        }
        case Mode::HOLD:
            break;
        case Mode::TIMER: {
            unsigned long elapsed = millis() - timerStartTime;
            if (elapsed >= timerDuration) {
                setOff();
                if (onComplete) onComplete();
            }
            break;
        }
    }
}

void HeaterModeManager::setOnCompleteCallback(void (*cb)()) { onComplete = cb; }
void HeaterModeManager::setOnFaultCallback(void (*cb)()) { onFault = cb; }

HeaterModeManager::Mode HeaterModeManager::getCurrentMode() const { return mode; }

String HeaterModeManager::modeToString(Mode mode) {
    switch (mode) {
        case Mode::OFF: return "OFF";
        case Mode::RAMP: return "RAMP";
        case Mode::HOLD: return "HOLD";
        case Mode::TIMER: return "TIMER";
    }
    return "UNKNOWN";
}

void HeaterModeManager::setMode(const String& modeStr) {
    if (modeStr.equalsIgnoreCase("OFF")) setOff();
    else if (modeStr.equalsIgnoreCase("HOLD")) setHold(heater.getTargetTemperature());
    // RAMP & TIMER need parameters; ignore here.
}

void HeaterModeManager::setMode(Mode modeVal) {
    switch (modeVal) {
        case Mode::OFF: setOff(); break;
        case Mode::HOLD: setHold(heater.getTargetTemperature()); break;
        default: mode = modeVal; break; // RAMP/TIMER maintained externally
    }
}

void HeaterModeManager::setTargetTemperature(float temp) {
    heater.setTargetTemperature(temp, 0.5f);
}

void HeaterModeManager::setRampParams(float startTemp, float endTemp, unsigned long durationSeconds) {
    rampStartTemp = startTemp;
    rampEndTemp = endTemp;
    rampDuration = durationSeconds * 1000UL;
}

void HeaterModeManager::setTimerParams(unsigned long durationSeconds, float targetTemp, bool useTemp) {
    timerDuration = durationSeconds * 1000UL;
    timerUseTemp = useTemp;
    if (useTemp) heater.setTargetTemperature(targetTemp, 0.5f);
}

void HeaterModeManager::setHoldTemp(float holdTemp) {
    heater.setTargetTemperature(holdTemp, 0.5f);
    mode = Mode::HOLD;
}
