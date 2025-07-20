/**
 * @file HeatingElement.h
 * @brief Temperature-controlled heating element with PT100 sensor
 * @author SmartPlate Project
 * @version 2.0
 */
#pragma once

#include <Arduino.h>
#include <Adafruit_MAX31865.h>
#include <functional>
#include <memory>

/**
 * @class HeatingElement
 * @brief Temperature-controlled heating element with PT100 sensor and bang-bang control
 * 
 * This class manages a heating element with temperature feedback using a PT100 sensor
 * via MAX31865 RTD-to-Digital converter. It provides bang-bang temperature control,
 * fault detection, and callback mechanisms for various events.
 */
class HeatingElement {
public:
    // Type aliases for better code readability
    using Callback = std::function<void()>;
    using TemperatureCallback = std::function<void(float)>;
    
    // Configuration constants
    static constexpr float DEFAULT_MAX_TEMP = 100.0f;
    static constexpr float DEFAULT_TOLERANCE = 1.0f;
    static constexpr float TEMP_CHANGE_THRESHOLD = 0.1f;
    static constexpr uint8_t DEFAULT_CS_PIN = 17;
    
    /**
     * @brief Constructor
     * @param relayPin GPIO pin connected to relay control
     * @param maxTempLimit Maximum safe temperature limit
     * @param filterSize Size of temperature averaging buffer (currently unused)
     * @param csPin SPI chip select pin for MAX31865 (default: 17)
     */
    HeatingElement(uint8_t relayPin, 
                   float maxTempLimit = DEFAULT_MAX_TEMP, 
                   int filterSize = 1,
                   uint8_t csPin = DEFAULT_CS_PIN);
    
    /**
     * @brief Destructor - cleans up allocated resources
     */
    ~HeatingElement();
    
    // Disable copy constructor and assignment operator
    HeatingElement(const HeatingElement&) = delete;
    HeatingElement& operator=(const HeatingElement&) = delete;
    
    /**
     * @brief Initialize the MAX31865 sensor (call once in setup)
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Update temperature reading and control logic (call frequently)
     */
    void update();
    
    /**
     * @brief Manually start the heater (if no fault condition)
     */
    void start();
    
    /**
     * @brief Manually stop the heater
     */
    void stop();
    
    /**
     * @brief Set target temperature for automatic control
     * @param target Target temperature in Celsius
     * @param tolerance Temperature tolerance for bang-bang control
     */
    void setTargetTemperature(float target, float tolerance = DEFAULT_TOLERANCE);
    
    /**
     * @brief Clear target temperature (disable automatic control)
     */
    void clearTargetTemperature();
    
    // Getters
    float getCurrentTemperature() const { return currentTemp; }
    bool isRunning() const { return isRunning_; }
    bool hasFault() const { return fault; }
    float getTargetTemperature() const { return targetTemp; }
    float getTargetTolerance() const { return targetTolerance; }
    bool hasTargetSet() const { return targetTempSet; }
    
    // Callback setters using std::function for better flexibility
    void setOnFaultCallback(const Callback& cb) { onFault = cb; }
    void setOnHeaterOnCallback(const Callback& cb) { onHeaterOn = cb; }
    void setOnHeaterOffCallback(const Callback& cb) { onHeaterOff = cb; }
    void setOnTargetReachedCallback(const Callback& cb) { onTargetReached = cb; }
    void setOnTemperatureChangedCallback(const TemperatureCallback& cb) { onTemperatureChanged = cb; }
    
private:
    /**
     * @brief Control relay state and update internal state
     * @param on true to turn relay on, false to turn off
     */
    void setRelay(bool on);
    
    /**
     * @brief Process new temperature reading
     * @param temp Temperature reading in Celsius
     */
    void processTemperatureReading(float temp);
    
    /**
     * @brief Check for fault conditions
     * @param temp Current temperature reading
     */
    void checkFaultConditions(float temp);
    
    /**
     * @brief Execute bang-bang temperature control
     */
    void executeBangBangControl();
    
    // Hardware configuration
    const uint8_t relayPin;
    const uint8_t csPin;
    const float maxTemp;
    
    // State variables
    bool isRunning_ = false;
    bool fault = false;
    
    // Temperature control
    float currentTemp = NAN;
    float targetTemp = 0.0f;
    float targetTolerance = DEFAULT_TOLERANCE;
    bool targetTempSet = false;
    bool targetReachedTriggered = false;
    
    // Temperature filtering (currently unused but kept for future enhancement)
    const int tempFilterSize;
    std::unique_ptr<float[]> tempBuffer;
    int tempIndex = 0;
    
    // Callbacks
    Callback onFault;
    Callback onHeaterOn;
    Callback onHeaterOff;
    Callback onTargetReached;
    TemperatureCallback onTemperatureChanged;
    
    // Hardware interface
    Adafruit_MAX31865 thermo;
};


