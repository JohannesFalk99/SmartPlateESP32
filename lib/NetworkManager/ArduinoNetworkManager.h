#pragma once

#include "INetworkManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

/**
 * @brief Arduino-based implementation of network management
 * 
 * This class implements the INetworkManager interface using Arduino
 * WiFi and OTA libraries. It isolates Arduino-specific APIs behind
 * the interface to enable future migration to ESP-IDF.
 */
class ArduinoNetworkManager : public INetworkManager {
public:
    /**
     * @brief Construct a new Arduino Network Manager
     */
    ArduinoNetworkManager();
    
    /**
     * @brief Initialize and connect to WiFi network
     * @param ssid WiFi network SSID
     * @param password WiFi network password
     * @return true if connection successful, false otherwise
     */
    bool connectWiFi(const char* ssid, const char* password) override;
    
    /**
     * @brief Initialize OTA update functionality
     * @param hostname Device hostname for OTA
     * @return true if OTA setup successful, false otherwise
     */
    bool setupOTA(const char* hostname) override;
    
    /**
     * @brief Handle OTA updates (call periodically)
     */
    void handleOTA() override;
    
    /**
     * @brief Check if WiFi is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const override;
    
    /**
     * @brief Get the local IP address
     * @return String representation of IP address
     */
    const char* getLocalIP() const override;

private:
    String ipAddress;  ///< Cached IP address string
    bool otaInitialized = false;  ///< OTA initialization state
    
    /**
     * @brief Setup OTA event callbacks
     */
    void setupOTACallbacks();
};
