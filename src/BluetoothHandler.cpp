#include "BluetoothHandler.h"
#include <Arduino.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <string>
#include "services/StateManager.h"
#include "services/WebServerManager.h"
#include "HeatingElement.h"
constexpr char SERVICE_UUID[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char TEMP_CHAR_UUID[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char MODE_CHAR_UUID[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char TIME_CHAR_UUID[] = "6E400004-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char CMD_CHAR_UUID[] = "6E400005-B5A3-F393-E0A9-E50E24DCCA9E";
BluetoothHandler::BluetoothHandler(StateManager* stateManager, HeatingElement* heatingElement): stateManager(stateManager), heatingElement(heatingElement) {}
void BluetoothHandler::begin(){ NimBLEDevice::init("SmartPlateESP32"); pServer = NimBLEDevice::createServer(); setupServices(); NimBLEDevice::startAdvertising(); }
void BluetoothHandler::setupServices(){ auto* pService = pServer->createService(SERVICE_UUID); pCmdCharacteristic = pService->createCharacteristic(CMD_CHAR_UUID, NIMBLE_PROPERTY::WRITE); pCmdCharacteristic->setCallbacks(new CommandCallbacks(this)); pTempCharacteristic = pService->createCharacteristic(TEMP_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY); pModeCharacteristic = pService->createCharacteristic(MODE_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY); pTimeCharacteristic = pService->createCharacteristic(TIME_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY); pService->start(); }
void BluetoothHandler::notifyTemperature(float temperature){ if(pTempCharacteristic){ std::string s = std::to_string(temperature); pTempCharacteristic->setValue(s); pTempCharacteristic->notify(); }}
void BluetoothHandler::notifyMode(const std::string& mode){ if(pModeCharacteristic){ pModeCharacteristic->setValue(mode); pModeCharacteristic->notify(); }}
void BluetoothHandler::notifyRunningTime(uint32_t seconds){ if(pTimeCharacteristic){ std::string s = std::to_string(seconds); pTimeCharacteristic->setValue(s); pTimeCharacteristic->notify(); }}
void BluetoothHandler::handleCommand(const std::string& cmd){ if(cmd.rfind("SET_TEMP:",0)==0){ float temp = static_cast<float>(atof(cmd.c_str()+9)); if(heatingElement) heatingElement->setTargetTemperature(temp,0.5f);} else if(cmd.rfind("SET_MODE:",0)==0){ /* TODO */ } }
void BluetoothHandler::CommandCallbacks::onWrite(NimBLECharacteristic* pCharacteristic){ std::string value = pCharacteristic->getValue(); if(!value.empty() && handler){ handler->handleCommand(value);} }
