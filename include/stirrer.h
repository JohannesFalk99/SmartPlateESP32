#include <cstdint>
#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

class MotorController {
public:
    MotorController(uint8_t relayPin);
    void begin();
    void update();
    void setRPM(uint16_t rpm);
    uint16_t getRPM() const;
private:
    uint8_t _pin;
    uint16_t _rpm;
    unsigned long _lastToggle;
    bool _state;
    uint16_t _onTime;
    uint16_t _offTime;
    void _computeDuty();
};

#endif
