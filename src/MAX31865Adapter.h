#pragma once
#include "ITemperatureSensor.h"
#include <Adafruit_MAX31865.h>

/**
 * @brief Adapter class for MAX31865 RTD temperature sensor
 * 
 * This class implements the ITemperatureSensor interface for the
 * Adafruit MAX31865 RTD sensor, configured for PT100 3-wire mode.
 */
class MAX31865Adapter : public ITemperatureSensor {
public:
    /**
     * @brief Construct a new MAX31865 Adapter
     * @param csPin Chip select pin for SPI communication
     */
    MAX31865Adapter(uint8_t csPin) : sensor(csPin) {}
    
    /**
     * @brief Initialize the MAX31865 sensor in 3-wire mode
     */
    void begin() { sensor.begin(MAX31865_3WIRE); }
    
    /**
     * @brief Read temperature from the sensor
     * @return float Temperature in degrees Celsius using PT100 calibration
     */
    float readTemperature() override {
        constexpr float RNOMINAL = 100.0f;
        constexpr float RREF = 424.0f;
        return sensor.temperature(RNOMINAL, RREF);
    }
    
    /**
     * @brief Read fault status from the sensor
     * @return uint8_t Fault code (0 if no fault)
     */
    uint8_t readFault() { return sensor.readFault(); }
    
    /**
     * @brief Clear any fault conditions
     */
    void clearFault() { sensor.clearFault(); }
private:
    Adafruit_MAX31865 sensor;
};