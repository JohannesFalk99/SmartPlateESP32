
// #include "stirrer.h"
// #include <esp32-hal.h>
// class StirrerModeManager
// {
// public:
//     enum Mode
//     {
//         OFF,
//         FIXED_RPM,
//         TIMER
//     };

//     StirrerModeManager(Stirrer &stirrer) : stirrer(stirrer) {}

//     void setOff()
//     {
//         mode = OFF;
//         stirrer.stop();
//     }

//     void setFixedRPM(int rpm)
//     {
//         mode = FIXED_RPM;
//         stirrer.setTargetRPM(rpm);
//         stirrer.start();
//     }

//     void setTimer(unsigned long durationSeconds, int rpm)
//     {
//         mode = TIMER;
//         timerDuration = durationSeconds * 1000UL;
//         timerStartTime = millis();
//         stirrer.setTargetRPM(rpm);
//         stirrer.start();
//     }

//     void update()
//     {
//         if (stirrer.hasFault())
//         {
//             if (onFault)
//                 onFault();
//             setOff();
//             return;
//         }

//         switch (mode)
//         {
//         case OFF:
//             break;

//         case FIXED_RPM:
//             // Running at fixed RPM
//             break;

//         case TIMER:
//         {
//             unsigned long elapsed = millis() - timerStartTime;
//             if (elapsed >= timerDuration)
//             {
//                 if (onComplete)
//                     onComplete();
//                 setOff();
//             }
//             break;
//         }
//         }
//     }

//     void setOnCompleteCallback(void (*cb)()) { onComplete = cb; }
//     void setOnFaultCallback(void (*cb)()) { onFault = cb; }

//     Mode getCurrentMode() const { return mode; }

// private:
//     Stirrer &stirrer;
//     Mode mode = OFF;

//     // Timer
//     unsigned long timerDuration = 0, timerStartTime = 0;

//     // Callbacks
//     void (*onComplete)() = nullptr;
//     void (*onFault)() = nullptr;
// };
