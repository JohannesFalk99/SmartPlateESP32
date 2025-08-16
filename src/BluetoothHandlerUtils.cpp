#include "services/BluetoothHandlerUtils.h"
#include "BluetoothHandler.h"
#include "services/StateManager.h"
#include "HeatingElement.h"
#include "services/WebServerManager.h"
#include <Arduino.h>
void updateBluetoothNotifications(BluetoothHandler* handler) {
    if (!handler) return;
    extern HeatingElement* gHeatingElement; // expected external heating element pointer
    if (gHeatingElement) {
        handler->notifyTemperature(gHeatingElement->getCurrentTemperature());
        handler->notifyMode(StateManager::getMode().c_str());
        handler->notifyRunningTime(StateManager::getRunningTimeSeconds());
    }
}
