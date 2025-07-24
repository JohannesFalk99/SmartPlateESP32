#include "HeaterModeManager.h"

HeaterModeManager::HeaterModeManager(HeatingElement &heater) : heater(heater) {}

void HeaterModeManager::setOff()
{
    mode = OFF;
    heater.stop();
}

void HeaterModeManager::setRamp(float startTemp, float endTemp, unsigned long durationSeconds)
{
    mode = RAMP;
    rampStartTemp = startTemp;
    rampEndTemp = endTemp;
    rampDuration = durationSeconds * 1000UL;
    rampStartTime = millis();
    heater.setTargetTemperature(endTemp, 0.5f); // Added tolerance argument
    heater.start();
}

void HeaterModeManager::setHold(float holdTemp)
{
    printf("HeaterModeManager: Setting hold mode at %.2f°C\n", holdTemp);
    mode = HOLD;
    heater.setTargetTemperature(holdTemp, 0.5f); // Added tolerance argument
    heater.start();
}

void HeaterModeManager::setTimer(unsigned long durationSeconds, float targetTemp, bool useTemp)
{
    mode = TIMER;
    timerDuration = durationSeconds * 1000UL;
    timerStartTime = millis();
    timerUseTemp = useTemp;
    if (useTemp)
    {
        heater.setTargetTemperature(targetTemp, 0.5f); // Added tolerance argument
    }
    heater.start();
}

void HeaterModeManager::update(float currentTemp)
{
    if (heater.hasFault())
    {
        if (onFault)
            onFault();
        setOff();
        return;
    }

    switch (mode)
    {
    case OFF:
        heater.stop();
        break;

    case RAMP:
    {
        unsigned long elapsed = millis() - rampStartTime;
        if (elapsed >= rampDuration)
        {
            heater.setTargetTemperature(rampEndTemp, 0.5f); // Added tolerance argument
            if (onComplete)
                onComplete();
            setOff();
        }
        else
        {
            float newTarget = rampStartTemp + (rampEndTemp - rampStartTemp) * ((float)elapsed / rampDuration);
            heater.setTargetTemperature(newTarget, 0.5f); // Added tolerance argument
            if (!heater.isRunningState())
                heater.start();
        }
        break;
    }

    case HOLD:
        if (!heater.isRunningState() && currentTemp < heater.getTargetTemperature() - 1.0f)
        {
            heater.start();
        }
        else if (heater.isRunningState() && currentTemp >= heater.getTargetTemperature())
        {
            heater.stop();
        }
        break;

    case TIMER:
    {
        if (!heater.isRunningState())
            heater.start();
        unsigned long elapsed = millis() - timerStartTime;
        if (elapsed >= timerDuration)
        {
            if (onComplete)
                onComplete();
            setOff();
        }
        break;
    }
    }
}

void HeaterModeManager::setOnCompleteCallback(void (*cb)()) { onComplete = cb; }
void HeaterModeManager::setOnFaultCallback(void (*cb)()) { onFault = cb; }
HeaterModeManager::Mode HeaterModeManager::getCurrentMode() const { return mode; }

String HeaterModeManager::modeToString(Mode mode)
{
    switch (mode)
    {
    case OFF:
        return "Off";
    case RAMP:
        return "Ramp";
    case HOLD:
        return "Hold";
    case TIMER:
        return "Timer";
    default:
        return "Unknown";
    }
}

void HeaterModeManager::setMode(const String& modeStr) {
    if (modeStr.equalsIgnoreCase("Off")) {
        setOff();
    } else if (modeStr.equalsIgnoreCase("Ramp")) {
        // Default ramp values, or you may want to require parameters
        setRamp(rampStartTemp, rampEndTemp, rampDuration / 1000UL);
    } else if (modeStr.equalsIgnoreCase("Hold")) {
        setHold(rampEndTemp); // Or use a default hold temp
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
