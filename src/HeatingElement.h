
// #pragma once

// #ifndef HEATINGELEMENT_H
// #define HEATINGELEMENT_H

// #include <Arduino.h>

// class HeatingElement
// {
// public:
//     using Callback = void (*)();

//     HeatingElement(uint8_t relayPin, float maxTempLimit = 300.0f, int filterSize = 10);
//     ~HeatingElement();

//     void start();
//     void stop();
    
//     void addTemperatureReading(float temp);
//     void setTargetTemperature(float target, float tolerance = 1.0f);
    
//     float getTargetTemperature() const;
//     float getCurrentTemperature() const;
//     float getFilteredTemperature() const;
//     bool isRunningState() const;
//     bool hasFault() const;

//     void setOnFaultCallback(Callback cb);
//     void setOnHeaterOnCallback(Callback cb);
//     void setOnHeaterOffCallback(Callback cb);
//     void setOnTargetReachedCallback(Callback cb);

// private:
//     uint8_t relayPin;
//     bool isRunning = false;
//     bool fault = false;

//     float currentTemp = 0.0f;
//     float filteredTemp = 0.0f;

//     float *tempBuffer;
//     int tempFilterSize;
//     int tempIndex = 0;

//     float maxTemp;
//     float targetTemp = 0;
//     float targetTolerance = 1.0f;
//     bool targetTempSet = false;
//     bool targetReachedTriggered = false;

//     Callback onFault = nullptr;
//     Callback onHeaterOn = nullptr;
//     Callback onHeaterOff = nullptr;
//     Callback onTargetReached = nullptr;

//     void setRelay(bool on);
// };

// #endif
#pragma once
#ifndef HEATINGELEMENT_H
#define HEATINGELEMENT_H

#include <Arduino.h>
#include <Adafruit_MAX31865.h>

class HeatingElement
{
public:
    using Callback = void (*)();

    // Constructor: relay pin, max temperature limit, filter buffer size
    HeatingElement(uint8_t relayPin, float maxTempLimit, int filterSize = 1);
    ~HeatingElement();

    // Initialize the MAX31865 sensor (call in setup)
    void begin();

    // Call frequently (e.g. in loop) to read temperature and control heater
    void update();

    // Start and stop heater manually
    void start();
    void stop();

    // Add a new temperature reading (called internally by update)
    void addTemperatureReading(float temp);

    // Set target temperature and tolerance for bang-bang control
    void setTargetTemperature(float target, float tolerance);

    // Getters
    float getCurrentTemperature() const;
    bool isRunningState() const;
    bool hasFault() const;
    float getTargetTemperature() const;

    // Callback setters
    void setOnFaultCallback(Callback cb);
    void setOnHeaterOnCallback(Callback cb);
    void setOnHeaterOffCallback(Callback cb);
    void setOnTargetReachedCallback(Callback cb);

    void setOnTemperatureChangedCallback(void (*cb)(float));

private:
    void setRelay(bool on);


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

    // MAX31865 PT100 sensor object (CS pin 5 hardcoded here)
    Adafruit_MAX31865 thermo;
};

#endif // HEATINGELEMENT_H
