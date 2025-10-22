# Quick Start Guide: Using the New Modular Architecture

## Overview
This guide shows how to use the newly modularized SmartPlate ESP32 firmware with NetworkManager and TaskManager.

## For Developers: Using the Modules

### NetworkManager - Handling WiFi and OTA

#### Basic Usage
```cpp
#include <ArduinoNetworkManager.h>

// Create instance
ArduinoNetworkManager networkManager;

// In setup()
void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi (non-blocking, 20s timeout)
    if (networkManager.connectWiFi("MySSID", "MyPassword")) {
        Serial.println("WiFi connected!");
        Serial.println(networkManager.getLocalIP());
    } else {
        Serial.println("WiFi failed - system will continue");
    }
    
    // Setup OTA updates
    if (networkManager.setupOTA("MyDevice")) {
        Serial.println("OTA ready");
    }
}

// In loop()
void loop() {
    networkManager.handleOTA();  // Process OTA requests
    // ... other code
}
```

#### Checking Connection Status
```cpp
if (networkManager.isConnected()) {
    // Do network operations
    sendDataToServer();
}
```

#### Error Handling
```cpp
// WiFi connection failure is non-fatal
if (!networkManager.connectWiFi(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("Running in offline mode");
    // System continues without network
}
```

### TaskManager - Managing FreeRTOS Tasks

#### Creating Tasks
```cpp
#include <TaskManager.h>

TaskManager taskManager;

// Define your task function
void myTask(void* parameter) {
    while (true) {
        // Task work here
        Serial.println("Task running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// In setup()
void setup() {
    TaskHandle_t handle = taskManager.createTask(
        "MyTask",           // Task name (for debugging)
        myTask,             // Task function
        NULL,               // Parameter to pass to task
        4096,               // Stack size (bytes)
        1,                  // Priority (0-24, higher = more priority)
        1                   // Core ID (0, 1, or tskNO_AFFINITY)
    );
    
    if (handle == NULL) {
        Serial.println("Failed to create task!");
    }
}
```

#### Using Mutexes for Synchronization
```cpp
// Create mutex in setup()
SemaphoreHandle_t myMutex = taskManager.createMutex();

// In tasks, protect shared data
void task1(void* param) {
    while (true) {
        // Try to acquire mutex with 100ms timeout
        if (taskManager.takeMutex(myMutex, 100)) {
            // Critical section - safe to access shared data
            sharedVariable++;
            
            // Release mutex
            taskManager.giveMutex(myMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

#### Best Practices for Mutexes
```cpp
void stateUpdateTask(void* param) {
    while (true) {
        // Acquire mutex
        if (taskManager.takeMutex(stateMutex, 100)) {
            // Update state (quick operations only)
            updateInternalState();
            
            // Release mutex BEFORE doing I/O
            taskManager.giveMutex(stateMutex);
        }
        
        // Do I/O operations OUTSIDE critical section
        sendWebUpdate();  // This may take time
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## Common Patterns

### Pattern 1: Graceful Network Failure
```cpp
void setup() {
    // Initialize critical systems first
    heater.begin();
    sensor.begin();
    
    // Network is optional
    if (networkManager.connectWiFi(SSID, PASSWORD)) {
        networkManager.setupOTA("SmartPlate");
        startWebServer();
    } else {
        Serial.println("Running without network features");
    }
    
    // Continue with task creation
    createTasks();
}
```

### Pattern 2: Multiple Tasks with Shared State
```cpp
SemaphoreHandle_t dataMutex;
float sharedTemperature = 0.0;

void sensorTask(void* param) {
    while (true) {
        float temp = sensor.read();
        
        if (taskManager.takeMutex(dataMutex, 100)) {
            sharedTemperature = temp;
            taskManager.giveMutex(dataMutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void displayTask(void* param) {
    while (true) {
        float temp;
        
        if (taskManager.takeMutex(dataMutex, 100)) {
            temp = sharedTemperature;  // Copy while holding lock
            taskManager.giveMutex(dataMutex);
        }
        
        display.showTemperature(temp);  // Display outside lock
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### Pattern 3: Conditional Network Operations
```cpp
void dataLoggingTask(void* param) {
    while (true) {
        float data = collectData();
        
        // Try to send to server if connected
        if (networkManager.isConnected()) {
            sendToServer(data);
        } else {
            // Fallback: log locally
            saveToSDCard(data);
        }
        
        vTaskDelay(pdMS_TO_TICKS(60000));  // Every minute
    }
}
```

## Configuration

### Network Settings (Config.h)
```cpp
constexpr char WIFI_SSID[] = "YourSSID";
constexpr char WIFI_PASSWORD[] = "YourPassword";
constexpr char OTA_HOSTNAME[] = "ESP32-Device";
```

### Hardware Pins (Config.h)
```cpp
constexpr int CS_PIN = 5;           // SPI chip select
constexpr int RELAY_PIN = 5;        // Relay control
```

### Task Configuration
```cpp
// Stack sizes (adjust based on needs)
const uint32_t SENSOR_TASK_STACK = 4096;
const uint32_t WEB_TASK_STACK = 4096;
const uint32_t STATE_TASK_STACK = 4096;

// Task priorities (0-24, higher = more important)
const UBaseType_t SENSOR_PRIORITY = 2;   // High priority
const UBaseType_t WEB_PRIORITY = 1;      // Normal priority
const UBaseType_t STATE_PRIORITY = 1;    // Normal priority
```

## Debugging Tips

### Enable Verbose Logging
```cpp
void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);  // Enable ESP32 debug output
    
    // Your setup code
}
```

### Monitor Task Creation
```cpp
TaskHandle_t handle = taskManager.createTask(...);
Serial.printf("Task count: %d\n", taskManager.getTaskCount());
```

### Check Network Status
```cpp
void loop() {
    if (!networkManager.isConnected()) {
        Serial.println("Network disconnected!");
        // Attempt reconnect or handle gracefully
    }
    
    networkManager.handleOTA();
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

### Monitor Mutex Contention
```cpp
if (!taskManager.takeMutex(myMutex, 100)) {
    Serial.println("Mutex timeout - possible contention!");
    // Log or handle the contention
}
```

## Troubleshooting

### "Task creation failed"
- **Cause**: Insufficient memory or MAX_TASKS exceeded
- **Solution**: Reduce stack sizes or increase MAX_TASKS in TaskManager.h

### "WiFi connection failed"
- **Cause**: Wrong credentials, router offline, or signal weak
- **Solution**: Check credentials in Config.h, verify router, check signal strength

### "Mutex timeout"
- **Cause**: Deadlock or task holding lock too long
- **Solution**: Review critical sections, ensure giveMutex() is always called

### "OTA upload fails"
- **Cause**: WiFi unstable or OTA not initialized
- **Solution**: Ensure WiFi stable before OTA, check OTA initialization logs

## Next Steps

1. **Read MODULARIZATION.md** for architecture details
2. **Review REFACTORING_SUMMARY.md** for complete changes
3. **Check MIGRATION_TO_ESPIDF.md** for ESP-IDF migration info
4. **Experiment** with the new modules in your own code

## Examples

See `src/main.cpp` for complete working example using:
- ArduinoNetworkManager for WiFi and OTA
- TaskManager for three concurrent tasks (heater, web, state)
- Proper mutex usage for shared state
- Error handling for network failures

## Support

For issues or questions:
1. Check documentation in MODULARIZATION.md
2. Review verification checklist in VERIFICATION_CHECKLIST.md
3. Open GitHub issue with logs and code samples
