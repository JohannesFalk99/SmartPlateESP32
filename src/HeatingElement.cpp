
// #include "HeatingElement.h"

// HeatingElement::HeatingElement(uint8_t relayPin, float maxTempLimit, int filterSize)
//     : relayPin(relayPin), maxTemp(maxTempLimit), tempFilterSize(filterSize)
// {
//     pinMode(relayPin, OUTPUT);
//     digitalWrite(relayPin, LOW);
//     tempBuffer = new float[tempFilterSize]();
// }

// HeatingElement::~HeatingElement()
// {
//     delete[] tempBuffer;
// }

// void HeatingElement::start()
// {
    
//     if (!isRunning && !fault)
//     {
//         Serial.println("HeatingElement: Starting heater");
//         setRelay(true);
//         if (onHeaterOn)
//             onHeaterOn();
//     }
// }

// void HeatingElement::stop()
// {
//     if (isRunning)
//     {
//         setRelay(false);
//         if (onHeaterOff)
//             onHeaterOff();
//     }
// }


// void HeatingElement::addTemperatureReading(float temp)
// {

//     tempBuffer[tempIndex] = temp;
//     tempIndex = (tempIndex + 1) % tempFilterSize;

//     // Filtering is disabled
//     // float sum = 0;
//     // for (int i = 0; i < tempFilterSize; i++) {
//     //     sum += tempBuffer[i];
//     // }
//     // filteredTemp = sum / tempFilterSize;

//     currentTemp = temp;
//     printf("HeatingElement: Current Temp = %.2f\n", currentTemp);
//     // Fault detection based on raw temperature
//     if (currentTemp >= maxTemp)
//     {
//         Serial.println("HeatingElement: Fault detected - over temperature!");
//         fault = true;
//         stop();
//         if (onFault)
//             onFault();
//     }
//     else
//     {
//         fault = false;
//     }

//     // Bang-bang control based on currentTemp (filtering disabled)
//     if (targetTempSet && !fault)
//     {
//         Serial.printf("HeatingElement: Current Temp = %.2f, Target Temp = %.2f, Tolerance = %.2f\n",
//                       currentTemp, targetTemp, targetTolerance);
//         if (currentTemp < targetTemp - targetTolerance)
//         {
//             setRelay(true);
//         }
//         else if (currentTemp >= targetTemp + targetTolerance)
//         {
//             //Serial.println("HeatingElement: Target temperature reached, turning off heater");
//             setRelay(false);
//         }
//     }

//     // Callback if target temp reached (within lower bound)
//     if (isRunning && !fault && targetTempSet &&
//         currentTemp >= targetTemp - targetTolerance &&
//         !targetReachedTriggered)
//     {
//         targetReachedTriggered = true;
//         if (onTargetReached)
//             onTargetReached();
//     }
//     else if (currentTemp < targetTemp - targetTolerance)
//     {
//         targetReachedTriggered = false;
//     }
// }

// void HeatingElement::setTargetTemperature(float target, float tolerance)
// {
//     targetTemp = target;
//     targetTolerance = tolerance;
//     targetTempSet = true;
//     targetReachedTriggered = false;
// }

// float HeatingElement::getCurrentTemperature() const { return currentTemp; }
// // float HeatingElement::getFilteredTemperature() const { return filteredTemp; } // Disabled
// bool HeatingElement::isRunningState() const { return isRunning; }
// bool HeatingElement::hasFault() const { return fault; }
// float HeatingElement::getTargetTemperature() const { return targetTemp; }

// void HeatingElement::setOnFaultCallback(Callback cb) { onFault = cb; }
// void HeatingElement::setOnHeaterOnCallback(Callback cb) { onHeaterOn = cb; }
// void HeatingElement::setOnHeaterOffCallback(Callback cb) { onHeaterOff = cb; }
// void HeatingElement::setOnTargetReachedCallback(Callback cb) { onTargetReached = cb; }

// void HeatingElement::setRelay(bool on)
// {
//     digitalWrite(relayPin, on ? HIGH : LOW);
//     Serial.printf("HeatingElement: Relay %s\n", on ? "ON" : "OFF");

//     if (on && !isRunning)
//     {
//         isRunning = true;
//         if (onHeaterOn)
//             onHeaterOn();
//     }
//     else if (!on && isRunning)
//     {
//         isRunning = false;
//         if (onHeaterOff)
//             onHeaterOff();
//     }
// }

#include "HeatingElement.h"
#include <Adafruit_MAX31865.h>

// Constants for PT100 sensor and reference resistor
#define RREF 424.0f
#define RNOMINAL 100.0f

// Constructor: initialize relay pin, max temp, and temp buffer
HeatingElement::HeatingElement(uint8_t relayPin, float maxTempLimit, int filterSize)
    : relayPin(relayPin), maxTemp(maxTempLimit), tempFilterSize(filterSize), thermo(17) // CS pin 5
{
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    tempBuffer = new float[tempFilterSize]();
}

// Destructor: free temp buffer
HeatingElement::~HeatingElement()
{
    delete[] tempBuffer;
}

// Call once in setup() to initialize the MAX31865 sensor
void HeatingElement::begin()
{
    thermo.begin(MAX31865_3WIRE); // Change to 2WIRE or 4WIRE if needed
    Serial.println("MAX31865 sensor initialized");
}

// Call repeatedly in loop() to read temperature and update control
void HeatingElement::update()
{
    float temp = thermo.temperature(RNOMINAL, RREF);
    addTemperatureReading(temp);
    printf("HeatingElement: Current Temp = %.2f°C\n", temp);
    // Check sensor faults and clear if any
    uint8_t faultCode = thermo.readFault();
    if (faultCode)
    {
        Serial.print("MAX31865 Fault: 0x");
        Serial.println(faultCode, HEX);
        thermo.clearFault();
    }
}

// Starts the heater if no fault
void HeatingElement::start()
{
    if (!isRunning && !fault)
    {
        Serial.println("HeatingElement: Starting heater");
        setRelay(true);
        if (onHeaterOn)
            onHeaterOn();
    }
}

// Stops the heater
void HeatingElement::stop()
{
    if (isRunning)
    {
        setRelay(false);
        if (onHeaterOff)
            onHeaterOff();
    }
}

// Adds a new temperature reading, handles fault and bang-bang control
void HeatingElement::addTemperatureReading(float temp)
{
    tempBuffer[tempIndex] = temp;
    tempIndex = (tempIndex + 1) % tempFilterSize;

    // Filtering disabled - use raw reading
    currentTemp = temp;
    Serial.printf("HeatingElement: Current Temp = %.2f\n", currentTemp);

    // Fault detection - over temperature
    if (currentTemp >= maxTemp)
    {
        Serial.println("HeatingElement: Fault detected - over temperature!");
        fault = true;
        stop();
        if (onFault)
            onFault();
    }
    else
    {
        fault = false;
    }

    // Bang-bang control for heater relay
    if (targetTempSet && !fault)
    {
        Serial.printf("HeatingElement: Current Temp = %.2f, Target Temp = %.2f, Tolerance = %.2f\n",
                      currentTemp, targetTemp, targetTolerance);
        if (currentTemp < targetTemp - targetTolerance)
        {
            setRelay(true);
        }
        else if (currentTemp >= targetTemp + targetTolerance)
        {
            setRelay(false);
        }
    }

    // Target temperature reached callback
    if (isRunning && !fault && targetTempSet &&
        currentTemp >= targetTemp - targetTolerance &&
        !targetReachedTriggered)
    {
        targetReachedTriggered = true;
        if (onTargetReached)
            onTargetReached();
    }
    else if (currentTemp < targetTemp - targetTolerance)
    {
        targetReachedTriggered = false;
    }
}

// Sets target temperature and tolerance
void HeatingElement::setTargetTemperature(float target, float tolerance)
{
    targetTemp = target;
    targetTolerance = tolerance;
    targetTempSet = true;
    targetReachedTriggered = false;
}

// Getters
float HeatingElement::getCurrentTemperature() const { return currentTemp; }
bool HeatingElement::isRunningState() const { return isRunning; }
bool HeatingElement::hasFault() const { return fault; }
float HeatingElement::getTargetTemperature() const { return targetTemp; }

// Callback setters
void HeatingElement::setOnFaultCallback(Callback cb) { onFault = cb; }
void HeatingElement::setOnHeaterOnCallback(Callback cb) { onHeaterOn = cb; }
void HeatingElement::setOnHeaterOffCallback(Callback cb) { onHeaterOff = cb; }
void HeatingElement::setOnTargetReachedCallback(Callback cb) { onTargetReached = cb; }

// Relay control with status update and callback triggers
void HeatingElement::setRelay(bool on)
{
    digitalWrite(relayPin, on ? HIGH : LOW);
    Serial.printf("HeatingElement: Relay %s\n", on ? "ON" : "OFF");

    if (on && !isRunning)
    {
        isRunning = true;
        if (onHeaterOn)
            onHeaterOn();
    }
    else if (!on && isRunning)
    {
        isRunning = false;
        if (onHeaterOff)
            onHeaterOff();
    }
}
