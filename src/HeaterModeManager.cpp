#include "Managers/HeaterModeManager.h"
#include <algorithm>

HeaterModeManager::HeaterModeManager(HeatingElement &heater) : heater(heater) {}

void HeaterModeManager::setOff() {
    mode = OFF;
    heater.stop();
}

void HeaterModeManager::setRamp(float startTemp, float endTemp, unsigned long durationSeconds) {
    mode = RAMP;
    rampStartTemp = startTemp;
    rampEndTemp = endTemp;
    rampDuration = durationSeconds * 1000UL;
    rampStartTime = millis();
    heater.setTargetTemperature(endTemp, 0.5f);
    heater.start();
}

void HeaterModeManager::setHold(float holdTemp) {
    printf("HeaterModeManager: Setting hold mode at %.2f°C\n", holdTemp);
    mode = HOLD;
    heater.setTargetTemperature(holdTemp, 0.5f);
    heater.start();
}

void HeaterModeManager::setTimer(unsigned long durationSeconds, float targetTemp, bool useTemp) {
    mode = TIMER;
    timerDuration = durationSeconds * 1000UL; 
    timerStartTime = millis();
    timerUseTemp = useTemp;
    if (useTemp) {
        heater.setTargetTemperature(targetTemp, 0.5f);
    }
    heater.start();
}

void HeaterModeManager::update(float currentTemp) {
    // Check for heater fault
    if (heater.hasFault()) {
        if (onFault) onFault();
        setOff();
        return;
    }

    // Handle OFF mode early to reduce nesting
    if (mode == OFF) {
        heater.stop();
        return;
    }

    switch (mode) {
        case RAMP: {
            // Calculate elapsed time for ramp
            unsigned long elapsedMs = millis() - rampStartTime;
            if (elapsedMs >= rampDuration) {
                // Ramp complete
                heater.setTargetTemperature(rampEndTemp, 0.5f);
                if (onComplete) onComplete();
                setOff();
            } else {
                // Ramp in progress: interpolate target temperature
                float progress = static_cast<float>(elapsedMs) / rampDuration;
                float interpolatedTarget = rampStartTemp + (rampEndTemp - rampStartTemp) * progress;
                heater.setTargetTemperature(interpolatedTarget, 0.5f);
                if (!heater.isRunningState()) heater.start();
            }
            break;
        }
        case HOLD: {
            // Hold mode: maintain target temperature
            float target = heater.getTargetTemperature();
            if (!heater.isRunningState() && currentTemp < target - 1.0f) {
                heater.start();
            } else if (heater.isRunningState() && currentTemp >= target) {
                heater.stop();
            }
            break;
        }
        case TIMER: {
            // Timer mode: run for specified duration
            if (!heater.isRunningState()) heater.start();
            unsigned long elapsedMs = millis() - timerStartTime;
            if (elapsedMs >= timerDuration) {
                if (onComplete) onComplete();
                setOff();
            }
            break;
        }
        // No default needed; OFF handled above
    }
}

void HeaterModeManager::setOnCompleteCallback(void (*cb)()) { onComplete = cb; }
void HeaterModeManager::setOnFaultCallback(void (*cb)()) { onFault = cb; }
HeaterModeManager::Mode HeaterModeManager::getCurrentMode() const { return mode; }

String HeaterModeManager::modeToString(Mode mode) {
    switch (mode) {
        case OFF: return "Off";
        case RAMP: return "Ramp";
        case HOLD: return "Hold";
        case TIMER: return "Timer";
        default: return "Unknown";
    }
}

void HeaterModeManager::setMode(const String& modeStr) {
    if (modeStr.equalsIgnoreCase("Off")) {
        setOff();
    } else if (modeStr.equalsIgnoreCase("Ramp")) {
        setRamp(rampStartTemp, rampEndTemp, rampDuration / 1000UL);
    } else if (modeStr.equalsIgnoreCase("Hold")) {
        setHold(rampEndTemp);
    } else if (modeStr.equalsIgnoreCase("Timer")) {
        setTimer(timerDuration / 1000UL, rampEndTemp, timerUseTemp);
    }
}

void HeaterModeManager::setMode(Mode modeVal) {
    switch (modeVal) {
        case OFF:
            setOff();
            break;
        case RAMP:
            setRamp(rampStartTemp, rampEndTemp, rampDuration / 1000UL);
            break;
        case HOLD:
            setHold(rampEndTemp);
            break;
        case TIMER:
            setTimer(timerDuration / 1000UL, rampEndTemp, timerUseTemp);
            break;
    }
}

void HeaterModeManager::setTargetTemperature(float temp) {
    rampEndTemp = temp;
    if (mode == HOLD || mode == RAMP || (mode == TIMER && timerUseTemp)) {
        heater.setTargetTemperature(temp, 0.5f);
    }
}

void HeaterModeManager::setRampParams(float startTemp, float endTemp, unsigned long durationSeconds) {
    rampStartTemp = startTemp;
    rampEndTemp = endTemp;
    rampDuration = durationSeconds * 1000UL;
}

void HeaterModeManager::setTimerParams(unsigned long durationSeconds, float targetTemp, bool useTemp) {
    timerDuration = durationSeconds * 1000UL;
    timerUseTemp = useTemp;
    if (useTemp) {
        rampEndTemp = targetTemp;
    }
}

void HeaterModeManager::setHoldTemp(float holdTemp) {
    rampEndTemp = holdTemp;
    if (mode == HOLD) {
        heater.setTargetTemperature(holdTemp, 0.5f);
    }
}
