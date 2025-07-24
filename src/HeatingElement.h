#pragma once
#ifndef HEATINGELEMENT_H
#define HEATINGELEMENT_H

#include <Arduino.h>
#include <Adafruit_MAX31865.h>
#include "ITemperatureSensor.h"

class HeatingElement
{
public:
    using Callback = void (*)();

    // Constructor: relay pin, max temperature limit, filter buffer size
    HeatingElement(uint8_t relayPin, float maxTempLimit, int filterSize, ITemperatureSensor* sensor);
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
    void setRelayWithCallback(bool on, Callback cb, const char* msg = nullptr);

    void triggerIfChanged(void (*cb)(float), float prev, float curr);
    void checkOverTemperature();
    void bangBangControl();
    void checkTargetReached();

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
