#pragma once
#include <NimBLEDevice.h>
#include <functional>
#include <string>
class StateManager; class HeatingElement;
class BluetoothHandler {
public:
    BluetoothHandler(StateManager* stateManager, HeatingElement* heatingElement);
    void begin();
    void notifyTemperature(float temperature);
    void notifyMode(const std::string& mode);
    void notifyRunningTime(uint32_t seconds);
private:
    StateManager* stateManager; HeatingElement* heatingElement;
    NimBLEServer* pServer = nullptr; NimBLECharacteristic* pCmdCharacteristic = nullptr; NimBLECharacteristic* pTempCharacteristic = nullptr; NimBLECharacteristic* pModeCharacteristic = nullptr; NimBLECharacteristic* pTimeCharacteristic = nullptr;
    void setupServices(); void handleCommand(const std::string& cmd);
    class CommandCallbacks : public NimBLECharacteristicCallbacks { public: CommandCallbacks(BluetoothHandler* handler) : handler(handler) {} private: BluetoothHandler* handler; void onWrite(NimBLECharacteristic* pCharacteristic); };
};
