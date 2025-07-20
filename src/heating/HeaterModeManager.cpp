#include "heating/HeaterModeManager.h"
#include "config/Config.h"

HeaterModeManager::HeaterModeManager(HeatingElement& heater) : heater(heater) {
    DEBUG_PRINTLN("HeaterModeManager: Initialized");
}

void HeaterModeManager::setOff() {
    DEBUG_PRINTLN("HeaterModeManager: Setting OFF mode");
    mode = Mode::OFF;
    heater.stop();
    heater.clearTargetTemperature();
}

void HeaterModeManager::setRamp(float startTemp, float endTemp, unsigned long durationSeconds, float tolerance) {
    DEBUG_PRINTF("HeaterModeManager: Setting RAMP mode %.2f°C -> %.2f°C over %lus\n", 
                 startTemp, endTemp, durationSeconds);
    
    mode = Mode::RAMP;
    rampParams.startTemp = startTemp;
    rampParams.endTemp = endTemp;
    rampParams.duration = durationSeconds * 1000UL;
    rampParams.startTime = millis();
    rampParams.tolerance = tolerance;
    
    // Set initial target temperature
    heater.setTargetTemperature(startTemp, tolerance);
    heater.start();
}

void HeaterModeManager::setHold(float holdTemp, float tolerance) {
    DEBUG_PRINTF("HeaterModeManager: Setting HOLD mode at %.2f°C ± %.2f°C\n", holdTemp, tolerance);
    
    mode = Mode::HOLD;
    holdParams.targetTemp = holdTemp;
    holdParams.tolerance = tolerance;
    
    heater.setTargetTemperature(holdTemp, tolerance);
    heater.start();
}

void HeaterModeManager::setTimer(unsigned long durationSeconds, float targetTemp, bool useTemp, float tolerance) {
    DEBUG_PRINTF("HeaterModeManager: Setting TIMER mode for %lus%s\n", 
                 durationSeconds, useTemp ? " with temperature control" : "");
    
    mode = Mode::TIMER;
    timerParams.duration = durationSeconds * 1000UL;
    timerParams.startTime = millis();
    timerParams.targetTemp = targetTemp;
    timerParams.useTemp = useTemp;
    timerParams.tolerance = tolerance;
    
    if (useTemp) {
        heater.setTargetTemperature(targetTemp, tolerance);
    }
    heater.start();
}

void HeaterModeManager::update(float currentTemp) {
    // Check for fault condition first
    if (heater.hasFault()) {
        handleFault();
        return;
    }
    
    // Update based on current mode
    switch (mode) {
        case Mode::OFF:
            // Ensure heater is stopped
            if (heater.isRunning()) {
                heater.stop();
            }
            break;
            
        case Mode::RAMP:
            updateRampMode();
            break;
            
        case Mode::HOLD:
            updateHoldMode(currentTemp);
            break;
            
        case Mode::TIMER:
            updateTimerMode();
            break;
    }
}

void HeaterModeManager::updateRampMode() {
    unsigned long elapsed = millis() - rampParams.startTime;
    
    if (elapsed >= rampParams.duration) {
        // Ramp complete - set final target and trigger completion
        DEBUG_PRINTLN("HeaterModeManager: Ramp completed");
        heater.setTargetTemperature(rampParams.endTemp, rampParams.tolerance);
        
        if (onComplete) {
            onComplete();
        }
        setOff();
    } else {
        // Calculate current target temperature based on progress
        float newTarget = calculateRampTarget();
        heater.setTargetTemperature(newTarget, rampParams.tolerance);
        
        // Ensure heater is running
        if (!heater.isRunning()) {
            heater.start();
        }
    }
}

void HeaterModeManager::updateHoldMode(float currentTemp) {
    // The HeatingElement handles the bang-bang control automatically
    // We just need to ensure the heater stays active
    if (!heater.isRunning() && currentTemp < holdParams.targetTemp - HOLD_HYSTERESIS) {
        heater.start();
    }
}

void HeaterModeManager::updateTimerMode() {
    unsigned long elapsed = millis() - timerParams.startTime;
    
    if (elapsed >= timerParams.duration) {
        // Timer complete
        DEBUG_PRINTLN("HeaterModeManager: Timer completed");
        
        if (onComplete) {
            onComplete();
        }
        setOff();
    } else {
        // Ensure heater is running
        if (!heater.isRunning()) {
            heater.start();
        }
    }
}

void HeaterModeManager::handleFault() {
    DEBUG_PRINTLN("HeaterModeManager: Fault detected, switching to OFF mode");
    
    if (onFault) {
        onFault();
    }
    setOff();
}

float HeaterModeManager::calculateRampTarget() const {
    unsigned long elapsed = millis() - rampParams.startTime;
    float progress = static_cast<float>(elapsed) / static_cast<float>(rampParams.duration);
    
    // Clamp progress to [0, 1]
    progress = std::min(1.0f, std::max(0.0f, progress));
    
    return rampParams.startTemp + (rampParams.endTemp - rampParams.startTemp) * progress;
}

bool HeaterModeManager::isComplete() const {
    switch (mode) {
        case Mode::OFF:
            return true;
            
        case Mode::RAMP:
            return (millis() - rampParams.startTime) >= rampParams.duration;
            
        case Mode::HOLD:
            return false; // Hold mode runs indefinitely
            
        case Mode::TIMER:
            return (millis() - timerParams.startTime) >= timerParams.duration;
            
        default:
            return false;
    }
}

float HeaterModeManager::getProgress() const {
    switch (mode) {
        case Mode::OFF:
            return 1.0f;
            
        case Mode::RAMP: {
            unsigned long elapsed = millis() - rampParams.startTime;
            return std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(rampParams.duration));
        }
        
        case Mode::HOLD:
            return 0.0f; // Hold mode doesn't have progress
            
        case Mode::TIMER: {
            unsigned long elapsed = millis() - timerParams.startTime;
            return std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(timerParams.duration));
        }
        
        default:
            return 0.0f;
    }
}

unsigned long HeaterModeManager::getRemainingTime() const {
    switch (mode) {
        case Mode::OFF:
            return 0;
            
        case Mode::RAMP: {
            unsigned long elapsed = millis() - rampParams.startTime;
            return (elapsed >= rampParams.duration) ? 0 : (rampParams.duration - elapsed);
        }
        
        case Mode::HOLD:
            return 0; // Hold mode runs indefinitely
            
        case Mode::TIMER: {
            unsigned long elapsed = millis() - timerParams.startTime;
            return (elapsed >= timerParams.duration) ? 0 : (timerParams.duration - elapsed);
        }
        
        default:
            return 0;
    }
}

float HeaterModeManager::getCurrentTarget() const {
    switch (mode) {
        case Mode::OFF:
            return 0.0f;
            
        case Mode::RAMP:
            return calculateRampTarget();
            
        case Mode::HOLD:
            return holdParams.targetTemp;
            
        case Mode::TIMER:
            return timerParams.useTemp ? timerParams.targetTemp : 0.0f;
            
        default:
            return 0.0f;
    }
}
