#include "heating/HeatingElement.h"
#include "config/Config.h"
#include <Adafruit_MAX31865.h>

HeatingElement::HeatingElement(uint8_t relayPin, float maxTempLimit, int filterSize, uint8_t csPin)
    : relayPin(relayPin), csPin(csPin), maxTemp(maxTempLimit), tempFilterSize(filterSize), 
      tempBuffer(std::make_unique<float[]>(filterSize)), thermo(csPin) {
    
    // Initialize relay pin
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    
    // Initialize temperature buffer
    for (int i = 0; i < tempFilterSize; ++i) {
        tempBuffer[i] = 0.0f;
    }
    
    DEBUG_PRINTLN("HeatingElement: Initialized");
}

HeatingElement::~HeatingElement() {
    // Smart pointer automatically cleans up tempBuffer
    DEBUG_PRINTLN("HeatingElement: Destroyed");
}

bool HeatingElement::begin() {
    if (!thermo.begin(PT100_WIRES)) {
        DEBUG_PRINTLN("HeatingElement: Failed to initialize MAX31865 sensor");
        fault = true;
        return false;
    }
    
    DEBUG_PRINTLN("HeatingElement: MAX31865 sensor initialized successfully");
    return true;
}

void HeatingElement::update() {
    // Read temperature from sensor
    float temp = thermo.temperature(PT100_RNOMINAL, PT100_RREF);
    
    // Check for sensor faults
    uint8_t faultCode = thermo.readFault();
    if (faultCode) {
        DEBUG_PRINTF("HeatingElement: MAX31865 Fault: 0x%02X\n", faultCode);
        thermo.clearFault();
        fault = true;
        stop();
        if (onFault) {
            onFault();
        }
        return;
    }
    
    // Process the temperature reading
    processTemperatureReading(temp);
    
    DEBUG_PRINTF("HeatingElement: Current Temp = %.2f°C\n", temp);
}

void HeatingElement::start() {
    if (fault) {
        DEBUG_PRINTLN("HeatingElement: Cannot start - fault condition exists");
        return;
    }
    
    if (!isRunning_) {
        DEBUG_PRINTLN("HeatingElement: Starting heater");
        setRelay(true);
    }
}

void HeatingElement::stop() {
    if (isRunning_) {
        DEBUG_PRINTLN("HeatingElement: Stopping heater");
        setRelay(false);
    }
}

void HeatingElement::processTemperatureReading(float temp) {
    // Store in buffer (for future filtering implementation)
    tempBuffer[tempIndex] = temp;
    tempIndex = (tempIndex + 1) % tempFilterSize;
    
    // Store previous temperature for change detection
    float previousTemp = currentTemp;
    currentTemp = temp;
    
    // Trigger temperature changed callback if value changed significantly
    if (onTemperatureChanged && 
        (std::isnan(previousTemp) || std::abs(currentTemp - previousTemp) > TEMP_CHANGE_THRESHOLD)) {
        onTemperatureChanged(currentTemp);
    }
    
    // Check for fault conditions
    checkFaultConditions(temp);
    
    // Execute temperature control if no fault
    if (!fault) {
        executeBangBangControl();
    }
}

void HeatingElement::checkFaultConditions(float temp) {
    // Check for over-temperature condition
    if (temp >= maxTemp) {
        DEBUG_PRINTF("HeatingElement: Over-temperature fault! Temp=%.2f°C, Max=%.2f°C\n", temp, maxTemp);
        fault = true;
        stop();
        if (onFault) {
            onFault();
        }
        return;
    }
    
    // Check for invalid temperature readings
    if (std::isnan(temp) || temp < -50.0f || temp > 500.0f) {
        DEBUG_PRINTF("HeatingElement: Invalid temperature reading: %.2f°C\n", temp);
        fault = true;
        stop();
        if (onFault) {
            onFault();
        }
        return;
    }
    
    // Clear fault if temperature is back to normal
    if (fault && temp < maxTemp - 5.0f) { // 5°C hysteresis
        DEBUG_PRINTLN("HeatingElement: Fault cleared");
        fault = false;
    }
}

void HeatingElement::executeBangBangControl() {
    if (!targetTempSet) {
        return;
    }
    
    DEBUG_PRINTF("HeatingElement: Bang-bang control - Current=%.2f°C, Target=%.2f°C, Tolerance=%.2f°C\n",
                 currentTemp, targetTemp, targetTolerance);
    
    // Bang-bang control logic
    if (currentTemp < targetTemp - targetTolerance) {
        setRelay(true);
    } else if (currentTemp >= targetTemp + targetTolerance) {
        setRelay(false);
    }
    
    // Check if target temperature reached for the first time
    if (isRunning_ && targetTempSet &&
        currentTemp >= targetTemp - targetTolerance &&
        currentTemp <= targetTemp + targetTolerance &&
        !targetReachedTriggered) {
        
        targetReachedTriggered = true;
        DEBUG_PRINTLN("HeatingElement: Target temperature reached");
        if (onTargetReached) {
            onTargetReached();
        }
    } else if (currentTemp < targetTemp - targetTolerance) {
        targetReachedTriggered = false;
    }
}

void HeatingElement::setTargetTemperature(float target, float tolerance) {
    targetTemp = target;
    targetTolerance = tolerance;
    targetTempSet = true;
    targetReachedTriggered = false;
    
    DEBUG_PRINTF("HeatingElement: Target temperature set to %.2f°C ± %.2f°C\n", target, tolerance);
}

void HeatingElement::clearTargetTemperature() {
    targetTempSet = false;
    targetReachedTriggered = false;
    DEBUG_PRINTLN("HeatingElement: Target temperature cleared");
}

// Note: Getters and callback setters are now inline in the header file

void HeatingElement::setRelay(bool on) {
    digitalWrite(relayPin, on ? HIGH : LOW);
    DEBUG_PRINTF("HeatingElement: Relay %s\n", on ? "ON" : "OFF");
    
    // Update state and trigger callbacks
    if (on && !isRunning_) {
        isRunning_ = true;
        if (onHeaterOn) {
            onHeaterOn();
        }
    } else if (!on && isRunning_) {
        isRunning_ = false;
        if (onHeaterOff) {
            onHeaterOff();
        }
    }
}
