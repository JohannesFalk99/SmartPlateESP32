#pragma once

// WiFi Configuration
// TODO: Move these to environment variables or external config file
#ifndef WIFI_SSID
#define WIFI_SSID "NETGEAR67"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "IHaveATinyPenis"
#endif

// OTA Configuration
#define OTA_HOSTNAME "ESP32-Heater"

// PT100 sensor configuration
#define PT100_RREF 424.0f
#define PT100_RNOMINAL 100.0f
#define PT100_WIRES MAX31865_3WIRE  // Change to 2WIRE or 4WIRE if needed

// Hardware Configuration
#define RELAY_PIN 5
#define MAX_TEMP_LIMIT 70.0f
#define TEMP_FILTER_SIZE 5

// Network Configuration
#define SERIAL_SERVER_PORT 23

// Timing Configuration
#define UPDATE_INTERVAL_MS 500
#define WIFI_CONNECT_TIMEOUT_MS 30000  // 30 seconds
#define WIFI_CONNECT_DELAY_MS 500

// System Configuration
#define SERIAL_BAUD_RATE 115200

// Debug Configuration
#define DEBUG_ENABLED true

#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(format, ...) Serial.printf(format, __VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(format, ...)
#endif
