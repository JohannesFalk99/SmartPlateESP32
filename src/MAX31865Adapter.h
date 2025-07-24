#pragma once
#include "ITemperatureSensor.h"
#include <Adafruit_MAX31865.h>

class MAX31865Adapter : public ITemperatureSensor {
public:
    MAX31865Adapter(uint8_t csPin) : sensor(csPin) {}
    void begin() { sensor.begin(MAX31865_3WIRE); }
    float readTemperature() override {
        constexpr float RNOMINAL = 100.0f;
        constexpr float RREF = 424.0f;
        return sensor.temperature(RNOMINAL, RREF);
    }
    uint8_t readFault() { return sensor.readFault(); }
    void clearFault() { sensor.clearFault(); }
private:
    Adafruit_MAX31865 sensor;
};