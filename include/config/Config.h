/**
 * @file Config.h
 * @brief System-wide configuration constants for SmartPlate ESP32
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint> // For fixed-width integer types

// WiFi credentials
constexpr char WIFI_SSID[] = "NETGEAR67";           ///< WiFi network SSID
constexpr char WIFI_PASSWORD[] = "IHaveATinyPenis"; ///< WiFi network password

// Server settings
constexpr uint16_t SERVER_PORT = 80;        ///< HTTP server port
constexpr char WEBSOCKET_PATH[] = "/ws";    ///< WebSocket endpoint path

// Alert thresholds
constexpr float ALERT_TEMP_THRESHOLD = 85.0f;   ///< Temperature alert threshold in degrees Celsius
constexpr int ALERT_RPM_THRESHOLD = 1500;        ///< RPM alert threshold
constexpr int ALERT_TIMER_THRESHOLD = 3600;      ///< Timer alert threshold in seconds

// History and event buffer sizes
constexpr int HISTORY_SIZE = 100;   ///< Maximum number of history entries
constexpr int MAX_EVENTS = 50;      ///< Maximum number of event log entries

/**
 * @brief Namespace containing mode name strings
 */
namespace Modes {
    constexpr char HOLD[] = "Hold";                         ///< Hold temperature mode
    constexpr char RAMP[] = "Ramp";                         ///< Temperature ramp mode
    constexpr char TIMER[] = "Timer";                       ///< Timer mode
    constexpr char RECRYSTALLIZATION[] = "Recrystallization"; ///< Recrystallization mode
    constexpr char OFF[] = "Off";                           ///< Off mode
}

// Other constants
constexpr float DEFAULT_RAMP_RATE = 1.0f;   ///< Default temperature ramp rate in degrees/second

// Hardware Configuration
constexpr int RELAY_PIN = 5;                ///< GPIO pin for relay control
constexpr float MAX_TEMP_LIMIT = 70.0f;     ///< Maximum safe temperature in degrees Celsius
constexpr int TEMP_FILTER_SIZE = 5;         ///< Size of temperature filter buffer

// System Configuration
constexpr unsigned long UPDATE_INTERVAL_MS = 500;   ///< System state update interval in milliseconds
constexpr int RPM_MIN = 100;                        ///< Minimum RPM value
constexpr int RPM_MAX = 200;                        ///< Maximum RPM value
constexpr int RPM_INCREMENT = 10;                   ///< RPM increment step
constexpr uint16_t SERIAL_TCP_PORT = 23;            ///< TCP port for remote serial (telnet)

#endif // CONFIG_H
