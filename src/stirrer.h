#pragma once
#include <Arduino.h> // <-- Add this line for uint8_t etc.
#include <functional>

class Stirrer
{
public:
    using Callback = void (*)();

    Stirrer(uint8_t motorPin, float maxRPMLimit = 3000.0f);

    void start();
    void stop();
    void setSpeed(uint8_t speed);
    void updateRPM(float rpm);
    void setTargetRPM(float rpm, float tolerance = 50.0f);

    float getRPM() const;
    bool isRunningState() const;
    bool hasFault() const;

    void setOnFaultCallback(Callback cb);
    void setOnMotorStartCallback(Callback cb);
    void setOnMotorStopCallback(Callback cb);
    void setOnTargetReachedCallback(Callback cb);

private:
    uint8_t motorPin;
    bool isRunning = false;
    bool fault = false;

    uint8_t motorSpeed = 128;
    float currentRPM = 0.0f;

    float maxRPM;
    float minSafeRPM = 100.0f;

    float targetRPM = 0;
    float targetTolerance = 50.0f;
    bool targetRPMSet = false;
    bool targetReachedTriggered = false;

    Callback onFault = nullptr;
    Callback onMotorStart = nullptr;
    Callback onMotorStop = nullptr;
    Callback onTargetReached = nullptr;
};