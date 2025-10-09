# Migration Guide: Arduino to ESP-IDF

## Overview
This guide describes how to migrate the SmartPlate firmware from Arduino framework to native ESP-IDF, leveraging the modular architecture introduced in the recent refactoring.

## Why Migrate to ESP-IDF?

### Benefits
1. **Better Performance**: Direct hardware access without Arduino abstraction overhead
2. **More Control**: Fine-grained control over FreeRTOS, networking, and peripherals
3. **Professional Features**: Built-in support for secure boot, flash encryption, power management
4. **Better Documentation**: Comprehensive ESP-IDF documentation and examples
5. **Industry Standard**: ESP-IDF is Espressif's official framework for production systems

### Trade-offs
- **Steeper Learning Curve**: More complex API compared to Arduino simplicity
- **More Code**: Some operations require more boilerplate
- **Breaking Changes**: Regular updates between ESP-IDF versions

## Current Architecture Enables Easy Migration

Thanks to the recent modularization, most application code is already framework-agnostic:

### Already Framework-Independent ✓
- Heater control logic (`HeatingElement`)
- Mode management (`HeaterModeManager`)
- Temperature sensor interface (`ITemperatureSensor`)
- State management
- Business logic in main.cpp

### Need ESP-IDF Implementations
- Network operations (currently `ArduinoNetworkManager`)
- Task management (uses FreeRTOS, mostly compatible)
- Web server (currently ESPAsyncWebServer)
- File system (currently Arduino LittleFS wrapper)

## Migration Steps

### Phase 1: Create ESP-IDF NetworkManager

**File**: `lib/NetworkManager/ESPIDFNetworkManager.h`

```cpp
#pragma once
#include "INetworkManager.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_https_ota.h"

class ESPIDFNetworkManager : public INetworkManager {
public:
    ESPIDFNetworkManager();
    ~ESPIDFNetworkManager();
    
    bool connectWiFi(const char* ssid, const char* password) override;
    bool setupOTA(const char* hostname) override;
    void handleOTA() override;
    bool isConnected() const override;
    const char* getLocalIP() const override;

private:
    esp_netif_t* netif;
    bool connected;
    char ipAddress[16];
    
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);
};
```

**Implementation**: `lib/NetworkManager/ESPIDFNetworkManager.cpp`

```cpp
#include "ESPIDFNetworkManager.h"
#include "esp_log.h"

static const char* TAG = "ESPIDFNetworkManager";

ESPIDFNetworkManager::ESPIDFNetworkManager() : netif(nullptr), connected(false) {
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create default WiFi station interface
    netif = esp_netif_create_default_wifi_sta();
}

bool ESPIDFNetworkManager::connectWiFi(const char* ssid, const char* password) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                               &wifi_event_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, this));
    
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi connecting to %s...", ssid);
    
    // Wait for connection (with timeout)
    int retry = 0;
    while (!connected && retry < 20) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry++;
    }
    
    return connected;
}

void ESPIDFNetworkManager::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                              int32_t event_id, void* event_data) {
    ESPIDFNetworkManager* manager = static_cast<ESPIDFNetworkManager*>(arg);
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        manager->connected = false;
        ESP_LOGI(TAG, "Disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        sprintf(manager->ipAddress, IPSTR, IP2STR(&event->ip_info.ip));
        manager->connected = true;
        ESP_LOGI(TAG, "Got IP: %s", manager->ipAddress);
    }
}

bool ESPIDFNetworkManager::setupOTA(const char* hostname) {
    // Implement ESP-IDF OTA using esp_https_ota
    // This is more complex - typically requires a separate task
    ESP_LOGI(TAG, "OTA setup for hostname: %s", hostname);
    return true;
}

void ESPIDFNetworkManager::handleOTA() {
    // In ESP-IDF, OTA typically runs in its own task
    // This could check for updates periodically
}

bool ESPIDFNetworkManager::isConnected() const {
    return connected;
}

const char* ESPIDFNetworkManager::getLocalIP() const {
    return ipAddress;
}
```

### Phase 2: Update main.cpp

**Single line change**:

```cpp
// Before:
ArduinoNetworkManager networkManager;

// After:
ESPIDFNetworkManager networkManager;
```

That's it! The rest of main.cpp works as-is because it only uses the `INetworkManager` interface.

### Phase 3: Replace Web Server

The web server is more complex. Options:

1. **Use ESP-IDF HTTP Server** (`esp_http_server`)
   - Native component, well-supported
   - Requires rewriting `WebServerManager` implementation
   - WebSocket support via external library

2. **Port ESPAsyncWebServer**
   - Some ESP-IDF ports exist
   - May require updates for recent ESP-IDF versions

3. **Create WebServer Interface**
   - Similar to `INetworkManager`
   - Allows gradual migration

### Phase 4: Replace Arduino APIs

Replace remaining Arduino dependencies:

| Arduino API | ESP-IDF Equivalent |
|-------------|-------------------|
| `Serial.begin()` | `uart_driver_install()` + `uart_param_config()` |
| `millis()` | `esp_timer_get_time() / 1000` |
| `delay()` | `vTaskDelay(pdMS_TO_TICKS())` |
| `digitalWrite()` | `gpio_set_level()` |
| `analogRead()` | `adc_oneshot_read()` |

### Phase 5: Update Build System

**Convert to ESP-IDF CMake**:

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(SmartPlateESP32)
```

Create `main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "main.cpp"
         "HeatingElement.cpp"
         "HeaterModeManager.cpp"
         "WebServerManager.cpp"
         # ... other sources
    INCLUDE_DIRS "."
                 "../include"
    REQUIRES esp_wifi esp_netif esp_https_ota esp_http_server
)
```

## Testing Strategy

1. **Unit Tests**: Create mocks for ESP-IDF components
2. **Integration Tests**: Test on actual hardware incrementally
3. **Parallel Development**: Keep Arduino version working during migration
4. **Feature Flags**: Use `#ifdef ARDUINO` vs `#ifdef ESP_IDF` during transition

## Timeline Estimate

- **Week 1**: Implement `ESPIDFNetworkManager`, test WiFi connectivity
- **Week 2**: Implement ESP-IDF OTA, test firmware updates
- **Week 3**: Replace web server (biggest effort)
- **Week 4**: Replace remaining Arduino APIs, cleanup
- **Week 5**: Testing, optimization, documentation

## Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP-IDF WiFi API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [ESP-IDF HTTP Server](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html)
- [ESP-IDF OTA](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_ota_ops.html)

## Conclusion

The modular architecture introduced in the recent refactoring makes ESP-IDF migration straightforward:

✓ **Most code unchanged**: Business logic, heater control, mode management
✓ **Clear migration path**: Replace implementations behind interfaces
✓ **Incremental migration**: Can migrate module-by-module
✓ **Reduced risk**: Interface contracts ensure compatibility

The hardest part is the web server, but even that has clear options and can be tackled independently of other modules.
