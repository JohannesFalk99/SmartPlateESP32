#include "managers/StateManager.h"
#include "managers/WebServerManager.h"
#include "utilities/SerialRemote.h"
void StateManager::updateState(float temperature, int rpm, const String &mode, float tempSetpoint, int rpmSetpoint, int duration, HeaterModeManager* modeManager)
{
    state.temperature = temperature;
    state.rpm = rpm;
    state.mode = mode;
    state.tempSetpoint = tempSetpoint;
    state.rpmSetpoint = rpmSetpoint;
    state.duration = duration;

    if (modeManager)
    {
        modeManager->setMode(mode);
        if (mode.equalsIgnoreCase("Hold")) {
            modeManager->setHoldTemp(tempSetpoint);
        } else if (mode.equalsIgnoreCase("Ramp")) {
            modeManager->setRampParams(tempSetpoint, tempSetpoint, duration);
        } else if (mode.equalsIgnoreCase("Timer")) {
            modeManager->setTimerParams(duration, tempSetpoint, true);
        }
        modeManager->setTargetTemperature(tempSetpoint);
    }
    logMessagef(LogLevel::INFO, "[StateManager] Updated state: Temp=%.2f°C, RPM=%d, Mode=%s", state.temperature, state.rpm, state.mode.c_str());
}

void StateManager::logState()
{
    logMessagef(LogLevel::INFO, "[StateManager] Current state: Temp=%.2f°C, RPM=%d, Mode=%s, TempSetpoint=%.2f, RpmSetpoint=%d, Duration=%d", state.temperature, state.rpm, state.mode.c_str(), state.tempSetpoint, state.rpmSetpoint, state.duration);
}