#pragma once

#pragma once

#include <Arduino.h>
#include <Adafruit_MAX31865.h>
#include "ITemperatureSensor.h"

class HeatingElement
{
public:
    using Callback = void (*)();

    HeatingElement(uint8_t relayPin, float maxTempLimit, int filterSize, ITemperatureSensor* sensor);
    ~HeatingElement();

    void begin();
    void update();
    void start();
    void stop();

    void addTemperatureReading(float temp);
    void setTargetTemperature(float target, float tolerance);

    float getCurrentTemperature() const;
    bool isRunningState() const;
    bool hasFault() const;
    float getTargetTemperature() const;

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

    uint8_t relayPin;
    bool isRunning = false;

    float maxTemp;
    float targetTemp = 0.0f;
    float targetTolerance = 0.0f;
    bool targetTempSet = false;
    bool targetReachedTriggered = false;

    bool fault = false;

    int tempFilterSize;
    float *tempBuffer = nullptr;
    int tempIndex = 0;
    float currentTemp = 0.0f;

    Callback onFault = nullptr;
    Callback onHeaterOn = nullptr;
    Callback onHeaterOff = nullptr;
    Callback onTargetReached = nullptr;

    ITemperatureSensor* tempSensor;
};

#endif // HEATINGELEMENT_H
