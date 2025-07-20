#ifndef CONFIG_H
#define CONFIG_H

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

#endif // CONFIG_H
