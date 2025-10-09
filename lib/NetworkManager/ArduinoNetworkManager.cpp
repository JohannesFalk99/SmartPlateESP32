#include "ArduinoNetworkManager.h"

ArduinoNetworkManager::ArduinoNetworkManager() : ipAddress(""), otaInitialized(false) {}

bool ArduinoNetworkManager::connectWiFi(const char* ssid, const char* password) {
    Serial.println("[NetworkManager] Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Non-blocking connection with timeout
    unsigned long startAttempt = millis();
    const unsigned long timeout = 20000; // 20 second timeout
    
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < timeout) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        ipAddress = WiFi.localIP().toString();
        Serial.printf("[NetworkManager] Connected! IP: %s\n", ipAddress.c_str());
        return true;
    } else {
        Serial.println("[NetworkManager] WiFi connection failed!");
        return false;
    }
}

bool ArduinoNetworkManager::setupOTA(const char* hostname) {
    if (!isConnected()) {
        Serial.println("[NetworkManager] Cannot setup OTA - WiFi not connected");
        return false;
    }
    
    ArduinoOTA.setHostname(hostname);
    setupOTACallbacks();
    ArduinoOTA.begin();
    
    otaInitialized = true;
    Serial.printf("[NetworkManager] OTA initialized with hostname: %s\n", hostname);
    return true;
}

void ArduinoNetworkManager::setupOTACallbacks() {
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.printf("[OTA] Update started: %s\n", type.c_str());
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update complete");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR: Serial.println("Auth Failed"); break;
            case OTA_BEGIN_ERROR: Serial.println("Begin Failed"); break;
            case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
            case OTA_END_ERROR: Serial.println("End Failed"); break;
            default: Serial.println("Unknown Error"); break;
        }
    });
}

void ArduinoNetworkManager::handleOTA() {
    if (otaInitialized) {
        ArduinoOTA.handle();
    }
}

bool ArduinoNetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

const char* ArduinoNetworkManager::getLocalIP() const {
    return ipAddress.c_str();
}
