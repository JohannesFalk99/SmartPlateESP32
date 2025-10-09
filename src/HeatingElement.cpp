#include "hardware/HeatingElement.h"
#include <Adafruit_MAX31865.h>
#include <MAX31865Adapter.h>
#include "utilities/SerialRemote.h"
// Constants for PT100 sensor and reference resistor
constexpr float RREF = 424.0f;
constexpr float RNOMINAL = 100.0f;

HeatingElement::HeatingElement(uint8_t relayPin, float maxTempLimit, int filterSize, ITemperatureSensor* sensor)
    : relayPin(relayPin), maxTemp(maxTempLimit), tempFilterSize(filterSize), tempSensor(sensor)
{
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    tempBuffer = new float[tempFilterSize]();
    currentTemp = NAN;
    onTemperatureChanged = nullptr;
}

HeatingElement::~HeatingElement()
{
    delete[] tempBuffer;
}

void HeatingElement::begin()
{
    // Assume tempSensor is a MAX31865Adapter if you call begin()
    auto* max = static_cast<MAX31865Adapter*>(tempSensor);
    if (max) max->begin();
}

void HeatingElement::update()
{
    float temp = tempSensor->readTemperature();
    addTemperatureReading(temp);

    // Assume tempSensor is a MAX31865Adapter if you want to access fault methods
    auto* max = static_cast<MAX31865Adapter*>(tempSensor);
    if (max) {
        uint8_t faultCode = max->readFault();
        if (faultCode)
        {
            logMessagef(LogLevel::INFO, "MAX31865 Fault: 0x%02X", faultCode);
            max->clearFault();
        }
    }
}

void HeatingElement::start()
{
    if (!isRunning && !fault)
        setRelayWithCallback(true, onHeaterOn, "Starting heater");
}

void HeatingElement::stop()
{
    if (isRunning)
        setRelayWithCallback(false, onHeaterOff, nullptr);
}

void HeatingElement::addTemperatureReading(float temp)
{
    tempBuffer[tempIndex] = temp;
    tempIndex = (tempIndex + 1) % tempFilterSize;
    float previousTemp = currentTemp;
    currentTemp = temp;
    triggerIfChanged(onTemperatureChanged, previousTemp, currentTemp);
    checkOverTemperature();
    bangBangControl();
    checkTargetReached();
}

void HeatingElement::setTargetTemperature(float target, float tolerance)
{
    targetTemp = target;
    targetTolerance = tolerance;
    targetTempSet = true;
    targetReachedTriggered = false;
}

float HeatingElement::getCurrentTemperature() const { return currentTemp; }
bool HeatingElement::isRunningState() const { return isRunning; }
bool HeatingElement::hasFault() const { return fault; }
float HeatingElement::getTargetTemperature() const { return targetTemp; }

void HeatingElement::setOnFaultCallback(Callback cb) { onFault = cb; }
void HeatingElement::setOnHeaterOnCallback(Callback cb) { onHeaterOn = cb; }
void HeatingElement::setOnHeaterOffCallback(Callback cb) { onHeaterOff = cb; }
void HeatingElement::setOnTargetReachedCallback(Callback cb) { onTargetReached = cb; }
void HeatingElement::setOnTemperatureChangedCallback(void (*cb)(float)) { onTemperatureChanged = cb; }

void HeatingElement::setRelay(bool on)
{
    digitalWrite(relayPin, on ? HIGH : LOW);
    Serial.printf("HeatingElement: Relay %s\n", on ? "ON" : "OFF");
    updateRunningState(on);
}

// --- Helper functions ---

void HeatingElement::setRelayWithCallback(bool on, Callback cb, const char* msg)
{
    setRelay(on);
    if (msg) Serial.println(String("HeatingElement: ") + msg);
    if (cb) cb();
}

void HeatingElement::triggerIfChanged(void (*cb)(float), float prev, float curr)
{
    if (cb && (isnan(prev) || fabs(curr - prev) > 0.01f))
        cb(curr);
}

void HeatingElement::checkOverTemperature()
{
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
}

void HeatingElement::bangBangControl()
{
    if (targetTempSet && !fault)
    {
        if (currentTemp < targetTemp - targetTolerance)
            setRelay(true);
        else if (currentTemp >= targetTemp + targetTolerance)
            setRelay(false);
    }
}

void HeatingElement::checkTargetReached()
{
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

void HeatingElement::updateRunningState(bool relayOn)
{
    if (relayOn && !isRunning)
    {
        isRunning = true;
        if (onHeaterOn)
            onHeaterOn();
    }
    else if (!relayOn && isRunning)
    {
        isRunning = false;
        if (onHeaterOff)
            onHeaterOff();
    }
}


