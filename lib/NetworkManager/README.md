# NetworkManager Library

## Overview
The NetworkManager library provides an abstraction layer for network operations (WiFi and OTA) to decouple the SmartPlate firmware from specific networking framework implementations.

## Architecture

### Interface Layer
- **INetworkManager**: Abstract interface defining network operations
  - `connectWiFi()`: Establish WiFi connection
  - `setupOTA()`: Initialize Over-The-Air update functionality
  - `handleOTA()`: Process OTA update requests
  - `isConnected()`: Check connection status
  - `getLocalIP()`: Get device IP address

### Implementation Layer
- **ArduinoNetworkManager**: Arduino WiFi/OTA implementation
  - Uses Arduino WiFi library for network connectivity
  - Uses ArduinoOTA for firmware updates
  - Provides non-blocking WiFi connection with timeout
  - Configurable OTA callbacks for update monitoring

## Benefits

1. **Framework Decoupling**: Application code uses `INetworkManager` interface, not Arduino-specific APIs
2. **Future Migration**: Easy to add ESP-IDF implementation without changing application code
3. **Testability**: Interface can be mocked for unit testing
4. **Error Handling**: Graceful handling of connection failures
5. **Non-blocking**: WiFi connection has timeout, won't hang indefinitely

## Usage

```cpp
#include <ArduinoNetworkManager.h>

// Create manager instance
ArduinoNetworkManager networkManager;

// In setup():
if (networkManager.connectWiFi("MySSID", "MyPassword")) {
    Serial.println("WiFi connected!");
    networkManager.setupOTA("MyDevice");
}

// In loop():
networkManager.handleOTA();
```

## Future Enhancements

- Add ESP-IDF implementation (`ESPIDFNetworkManager`)
- Add connection monitoring and auto-reconnect
- Add network event callbacks
- Support for static IP configuration
- Support for multiple WiFi networks with fallback
