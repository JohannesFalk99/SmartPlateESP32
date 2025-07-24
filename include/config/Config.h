#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint> // For fixed-width integer types

// WiFi credentials
constexpr char WIFI_SSID[] = "NETGEAR67";
constexpr char WIFI_PASSWORD[] = "IHaveATinyPenis";

// Server settings
constexpr uint16_t SERVER_PORT = 80;
constexpr char WEBSOCKET_PATH[] = "/ws";

// Alert thresholds
constexpr float ALERT_TEMP_THRESHOLD = 85.0f;
constexpr int ALERT_RPM_THRESHOLD = 1500;
constexpr int ALERT_TIMER_THRESHOLD = 3600; // in seconds

// History and event buffer sizes
constexpr int HISTORY_SIZE = 100;
constexpr int MAX_EVENTS = 50;

// Mode strings
namespace Modes {
    constexpr char HOLD[] = "Hold";
    constexpr char RAMP[] = "Ramp";
    constexpr char TIMER[] = "Timer";
    constexpr char RECRYSTALLIZATION[] = "Recrystallization";
    constexpr char OFF[] = "Off";
}

// Other constants
constexpr float DEFAULT_RAMP_RATE = 1.0f;

// Hardware Configuration
constexpr int RELAY_PIN = 5;
constexpr float MAX_TEMP_LIMIT = 70.0f;
constexpr int TEMP_FILTER_SIZE = 5;

// System Configuration
constexpr unsigned long UPDATE_INTERVAL_MS = 500;
constexpr int RPM_MIN = 100;
constexpr int RPM_MAX = 200;
constexpr int RPM_INCREMENT = 10;
constexpr uint16_t SERIAL_TCP_PORT = 23;

#endif // CONFIG_H
