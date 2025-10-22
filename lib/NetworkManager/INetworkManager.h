#pragma once

/**
 * @brief Interface for network management functionality
 * 
 * This interface abstracts WiFi and OTA operations to decouple
 * the application from specific networking framework implementations.
 */
class INetworkManager {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~INetworkManager() = default;
    
    /**
     * @brief Initialize and connect to WiFi network
     * @param ssid WiFi network SSID
     * @param password WiFi network password
     * @return true if connection successful, false otherwise
     */
    virtual bool connectWiFi(const char* ssid, const char* password) = 0;
    
    /**
     * @brief Initialize OTA update functionality
     * @param hostname Device hostname for OTA
     * @return true if OTA setup successful, false otherwise
     */
    virtual bool setupOTA(const char* hostname) = 0;
    
    /**
     * @brief Handle OTA updates (call periodically)
     */
    virtual void handleOTA() = 0;
    
    /**
     * @brief Check if WiFi is connected
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Get the local IP address
     * @return String representation of IP address
     */
    virtual const char* getLocalIP() const = 0;
};
