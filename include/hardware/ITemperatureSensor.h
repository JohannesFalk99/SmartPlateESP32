#pragma once

/**
 * @brief Interface for temperature sensor implementations
 * 
 * This abstract class defines the interface that temperature sensors
 * must implement to be used with the heating element controller.
 */
class ITemperatureSensor {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ITemperatureSensor() = default;
    
    /**
     * @brief Read the current temperature from the sensor
     * @return float Temperature reading in degrees Celsius
     */
    virtual float readTemperature() = 0;
};