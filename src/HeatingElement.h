#pragma once
#ifndef HEATINGELEMENT_H
#define HEATINGELEMENT_H

#include <Arduino.h>
#include <Adafruit_MAX31865.h>
#include "ITemperatureSensor.h"

/**
 * @brief Controls a heating element with temperature monitoring and safety features
 * 
 * This class manages a heating element connected to a relay, monitors temperature
 * via a sensor interface, implements bang-bang control, and provides fault detection
 * and callback mechanisms.
 */
class HeatingElement
{
public:
    using Callback = void (*)();

    /**
     * @brief Construct a new Heating Element object
     * @param relayPin GPIO pin number for the relay control
     * @param maxTempLimit Maximum temperature limit for safety
     * @param filterSize Size of the temperature filter buffer
     * @param sensor Pointer to temperature sensor interface
     */
    HeatingElement(uint8_t relayPin, float maxTempLimit, int filterSize, ITemperatureSensor* sensor);
    
    /**
     * @brief Destroy the Heating Element object and clean up resources
     */
    ~HeatingElement();

    /**
     * @brief Initialize the temperature sensor (call in setup)
     */
    void begin();

    /**
     * @brief Update the heater state - call frequently (e.g. in loop)
     * 
     * Reads temperature, checks for faults, and controls heater based on current mode
     */
    void update();

    /**
     * @brief Manually start the heater
     */
    void start();
    
    /**
     * @brief Manually stop the heater
     */
    void stop();


    /**
     * @brief Add a new temperature reading to the filter buffer
     * @param temp Temperature value to add
     */
    void addTemperatureReading(float temp);

    /**
     * @brief Set target temperature and tolerance for bang-bang control
     * @param target Target temperature in degrees Celsius
     * @param tolerance Temperature tolerance for hysteresis
     */
    void setTargetTemperature(float target, float tolerance);

    /**
     * @brief Get the current temperature reading
     * @return float Current temperature in degrees Celsius
     */
    float getCurrentTemperature() const;
    
    /**
     * @brief Check if heater is currently running
     * @return true if heater is running
     * @return false if heater is stopped
     */
    bool isRunningState() const;
    
    /**
     * @brief Check if a fault condition has occurred
     * @return true if fault detected
     * @return false if no fault
     */
    bool hasFault() const;
    
    /**
     * @brief Get the target temperature setpoint
     * @return float Target temperature in degrees Celsius
     */
    float getTargetTemperature() const;

    /**
     * @brief Set callback function to be called when fault is detected
     * @param cb Function pointer to callback
     */
    void setOnFaultCallback(Callback cb);
    
    /**
     * @brief Set callback function to be called when heater turns on
     * @param cb Function pointer to callback
     */
    void setOnHeaterOnCallback(Callback cb);
    
    /**
     * @brief Set callback function to be called when heater turns off
     * @param cb Function pointer to callback
     */
    void setOnHeaterOffCallback(Callback cb);
    
    /**
     * @brief Set callback function to be called when target temperature is reached
     * @param cb Function pointer to callback
     */
    void setOnTargetReachedCallback(Callback cb);

    /**
     * @brief Set callback function to be called when temperature changes
     * @param cb Function pointer to callback with temperature parameter
     */
    void setOnTemperatureChangedCallback(void (*cb)(float));

private:
    /**
     * @brief Set the relay state (on/off)
     * @param on true to turn relay on, false to turn off
     */
    void setRelay(bool on);
    
    /**
     * @brief Set relay state and trigger callback
     * @param on true to turn relay on, false to turn off
     * @param cb Callback function to trigger
     * @param msg Optional message for logging
     */
    void setRelayWithCallback(bool on, Callback cb, const char* msg = nullptr);

    /**
     * @brief Trigger callback if value has changed
     * @param cb Callback function
     * @param prev Previous value
     * @param curr Current value
     */
    void triggerIfChanged(void (*cb)(float), float prev, float curr);
    
    /**
     * @brief Check if temperature exceeds maximum limit and handle fault
     */
    void checkOverTemperature();
    
    /**
     * @brief Implement bang-bang control algorithm
     */
    void bangBangControl();
    
    /**
     * @brief Check if target temperature has been reached
     */
    void checkTargetReached();

    /**
     * @brief Update the running state based on relay state
     * @param relayOn Current relay state
     */
    void updateRunningState(bool relayOn);

    void (*onTemperatureChanged)(float) = nullptr;

    // Relay pin and heater state
    uint8_t relayPin;
    bool isRunning = false;

    // Temperature control variables
    float maxTemp;
    float targetTemp = 0.0f;
    float targetTolerance = 0.0f;
    bool targetTempSet = false;
    bool targetReachedTriggered = false;

    // Fault state
    bool fault = false;

    // Temperature filtering buffer
    int tempFilterSize;
    float *tempBuffer = nullptr;
    int tempIndex = 0;

    // Current and filtered temperature
    float currentTemp = 0.0f;
    // float filteredTemp = 0.0f; // Filtering disabled in implementation

    // Callbacks
    Callback onFault = nullptr;
    Callback onHeaterOn = nullptr;
    Callback onHeaterOff = nullptr;
    Callback onTargetReached = nullptr;

   

    // Temperature sensor interface
    ITemperatureSensor* tempSensor;
};

#endif // HEATINGELEMENT_H
