// #include <Arduino.h>

// // Heating Element class - controls heating via a relay pin, reads temp sensor input externally
// class HeatingElement
// {
// public:
//     // Callback function types
//     using Callback = void (*)();

//     HeatingElement(uint8_t relayPin, float maxTempLimit = 300.0f, int filterSize = 10)
//         : relayPin(relayPin), maxTemp(maxTempLimit), tempFilterSize(filterSize)
//     {
//         pinMode(relayPin, OUTPUT);
//         digitalWrite(relayPin, LOW);
//         tempBuffer = new float[tempFilterSize]();
//     }

//     ~HeatingElement()
//     {
//         delete[] tempBuffer;
//     }

//     // Control methods
//     void start()
//     {
//         if (!isRunning && !fault)
//         {
//             digitalWrite(relayPin, HIGH);
//             isRunning = true;
//             if (onHeaterOn)
//                 onHeaterOn();
//         }
//     }

//     void stop()
//     {
//         if (isRunning)
//         {
//             digitalWrite(relayPin, LOW);
//             isRunning = false;
//             if (onHeaterOff)
//                 onHeaterOff();
//         }
//     }

//     // Add raw temperature reading (from sensor)
//     void addTemperatureReading(float temp)
//     {
//         // Insert into circular buffer
//         tempBuffer[tempIndex] = temp;
//         tempIndex = (tempIndex + 1) % tempFilterSize;

//         // Calculate moving average
//         float sum = 0;
//         for (int i = 0; i < tempFilterSize; i++)
//         {
//             sum += tempBuffer[i];
//         }
//         filteredTemp = sum / tempFilterSize;

//         currentTemp = temp;

//         // Check for fault condition (over-temperature)
//         if (filteredTemp >= maxTemp)
//         {
//             fault = true;
//             stop();
//             if (onFault)
//                 onFault();
//         }
//         else
//         {
//             fault = false;
//         }

//         // Trigger callback if target temp reached (with some tolerance)
//         if (isRunning && !fault && targetTempSet &&
//             filteredTemp >= targetTemp - targetTolerance &&
//             !targetReachedTriggered)
//         {
//             targetReachedTriggered = true;
//             if (onTargetReached)
//                 onTargetReached();
//         }
//         else if (filteredTemp < targetTemp - targetTolerance)
//         {
//             targetReachedTriggered = false; // reset if temp falls below target
//         }
//     }

//     // Set desired target temperature for callback use (optional)
//     void setTargetTemperature(float target, float tolerance = 1.0f)
//     {
//         targetTemp = target;
//         targetTolerance = tolerance;
//         targetTempSet = true;
//         targetReachedTriggered = false;
//     }

//     // Accessors
//     float getCurrentTemperature() const { return currentTemp; }
//     float getFilteredTemperature() const { return filteredTemp; }
//     bool isRunningState() const { return isRunning; }
//     bool hasFault() const { return fault; }

//     // Setters for callbacks
//     void setOnFaultCallback(Callback cb) { onFault = cb; }
//     void setOnHeaterOnCallback(Callback cb) { onHeaterOn = cb; }
//     void setOnHeaterOffCallback(Callback cb) { onHeaterOff = cb; }
//     void setOnTargetReachedCallback(Callback cb) { onTargetReached = cb; }

//     // Additional properties you might want to track or add:
//     // float powerLevel; // if PWM control supported in future
//     // unsigned long runTime; // track heating duration
//     // ...

// private:
//     uint8_t relayPin;
//     bool isRunning = false;
//     bool fault = false;

//     float currentTemp = 0.0f;
//     float filteredTemp = 0.0f;

//     // Filtering buffer
//     float *tempBuffer;
//     int tempFilterSize;
//     int tempIndex = 0;

//     // Safety and target temp
//     float maxTemp;
//     float targetTemp = 0;
//     float targetTolerance = 1.0f;
//     bool targetTempSet = false;
//     bool targetReachedTriggered = false;

//     // Callbacks
//     Callback onFault = nullptr;
//     Callback onHeaterOn = nullptr;
//     Callback onHeaterOff = nullptr;
//     Callback onTargetReached = nullptr;

//     // Internal helper
//     void setRelay(bool on)
//     {
//         digitalWrite(relayPin, on ? HIGH : LOW);
//     }
// };

// // Stirrer class - controls stirring motor, reads rpm from encoder or hall sensor externally
// class Stirrer
// {
// public:
//     // Callback types
//     using Callback = void (*)();

//     Stirrer(uint8_t motorPin, float maxRPMLimit = 3000.0f)
//         : motorPin(motorPin), maxRPM(maxRPMLimit)
//     {
//         pinMode(motorPin, OUTPUT);
//         digitalWrite(motorPin, LOW);
//     }

//     void start()
//     {
//         if (!isRunning)
//         {
//             analogWrite(motorPin, motorSpeed);
//             isRunning = true;
//             if (onMotorStart)
//                 onMotorStart();
//         }
//     }

//     void stop()
//     {
//         if (isRunning)
//         {
//             analogWrite(motorPin, 0);
//             isRunning = false;
//             if (onMotorStop)
//                 onMotorStop();
//         }
//     }

//     void setSpeed(uint8_t speed)
//     {
//         // speed from 0 to 255 (PWM)
//         motorSpeed = constrain(speed, 0, 255);
//         if (isRunning)
//         {
//             analogWrite(motorPin, motorSpeed);
//         }
//     }

//     // Set measured RPM (should be updated from external sensor)
//     void updateRPM(float rpm)
//     {
//         currentRPM = rpm;

//         // Check for fault if RPM is too low while running
//         if (isRunning && currentRPM < minSafeRPM)
//         {
//             fault = true;
//             stop();
//             if (onFault)
//                 onFault();
//         }
//         else
//         {
//             fault = false;
//         }

//         // Trigger callback if target RPM reached (with tolerance)
//         if (targetRPMSet && abs(currentRPM - targetRPM) <= targetTolerance && !targetReachedTriggered)
//         {
//             targetReachedTriggered = true;
//             if (onTargetReached)
//                 onTargetReached();
//         }
//         else if (abs(currentRPM - targetRPM) > targetTolerance)
//         {
//             targetReachedTriggered = false; // reset
//         }
//     }

//     void setTargetRPM(float rpm, float tolerance = 50.0f)
//     {
//         targetRPM = rpm;
//         targetTolerance = tolerance;
//         targetRPMSet = true;
//         targetReachedTriggered = false;
//     }

//     // Accessors
//     float getRPM() const { return currentRPM; }
//     bool isRunningState() const { return isRunning; }
//     bool hasFault() const { return fault; }

//     // Callback setters
//     void setOnFaultCallback(Callback cb) { onFault = cb; }
//     void setOnMotorStartCallback(Callback cb) { onMotorStart = cb; }
//     void setOnMotorStopCallback(Callback cb) { onMotorStop = cb; }
//     void setOnTargetReachedCallback(Callback cb) { onTargetReached = cb; }

//     // Additional properties you might want:
//     // unsigned long runTime;
//     // float motorCurrent; // if you have current sensor
//     // float motorVoltage; // if you monitor voltage

// private:
//     uint8_t motorPin;
//     bool isRunning = false;
//     bool fault = false;

//     uint8_t motorSpeed = 128; // default medium speed PWM (0-255)
//     float currentRPM = 0.0f;

//     // Safety
//     float maxRPM;
//     float minSafeRPM = 100.0f;

//     // Target RPM control
//     float targetRPM = 0;
//     float targetTolerance = 50.0f;
//     bool targetRPMSet = false;
//     bool targetReachedTriggered = false;

//     // Callbacks
//     Callback onFault = nullptr;
//     Callback onMotorStart = nullptr;
//     Callback onMotorStop = nullptr;
//     Callback onTargetReached = nullptr;
// };
