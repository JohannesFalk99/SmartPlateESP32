#include "stirrer.h"
#include <Arduino.h>

MotorController::MotorController(uint8_t relayPin)
    : _pin(relayPin), _rpm(0), _lastToggle(0), _state(false) {}

void MotorController::begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _computeDuty();
}

void MotorController::setRPM(uint16_t rpm) {
    _rpm = constrain(rpm, 0, 300);
    _computeDuty();
}

uint16_t MotorController::getRPM() const { return _rpm; }

void MotorController::_computeDuty() {
    uint16_t duty = static_cast<uint16_t>(map(_rpm, 0, 300, 0, 100));
    uint16_t cycle = 300; // ms
    _onTime = (cycle * duty) / 100;
    _offTime = cycle - _onTime;
}

void MotorController::update() {
    unsigned long now = millis();
    if (_state && now - _lastToggle >= _onTime) {
        digitalWrite(_pin, LOW);
        _state = false;
        _lastToggle = now;
    } else if (!_state && now - _lastToggle >= _offTime) {
        if (_onTime > 0) {
            digitalWrite(_pin, HIGH);
            _state = true;
            _lastToggle = now;
        }
    }
}
