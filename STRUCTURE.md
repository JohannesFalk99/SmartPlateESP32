# SmartPlate ESP32 Project Structure

This document describes the organization of the SmartPlate ESP32 project following PlatformIO best practices.

## Directory Structure

```
SmartPlateESP32/
├── src/                          # Application source files (.cpp only)
│   ├── main.cpp                  # Main application entry point
│   ├── HeatingElement.cpp        # Heating element controller implementation
│   ├── HeaterModeManager.cpp     # Mode management implementation
│   ├── WebServerManager.cpp      # Web server and WebSocket implementation
│   ├── StateManager.cpp          # System state management implementation
│   ├── NotepadManager.cpp        # Notes persistence implementation
│   ├── FileSystemExplorer.cpp    # File system web interface implementation
│   ├── WebServerActions.cpp      # WebSocket action handlers implementation
│   ├── SerialRemote.cpp          # TCP serial logging implementation
│   └── NotUsed/                  # Deprecated code (excluded from build)
│
├── include/                      # Public header files
│   ├── config/
│   │   └── Config.h              # System-wide configuration constants
│   ├── hardware/
│   │   ├── ITemperatureSensor.h  # Temperature sensor interface
│   │   └── HeatingElement.h      # Heating element controller
│   ├── managers/
│   │   ├── HeaterModeManager.h   # Operating modes (OFF, RAMP, HOLD, TIMER)
│   │   ├── WebServerManager.h    # Web server and WebSocket manager
│   │   ├── StateManager.h        # Global system state manager
│   │   └── NotepadManager.h      # Experiment notes manager
│   └── utilities/
│       ├── FileSystemExplorer.h  # LittleFS web interface
│       ├── SerialRemote.h        # Remote serial logging
│       └── WebServerActions.h    # WebSocket message handlers
│
├── lib/                          # Project-specific libraries
│   ├── MAX31865Adapter/          # Hardware driver library
│   │   ├── MAX31865Adapter.h     # PT100 RTD sensor adapter
│   │   └── library.json          # Library metadata
│   ├── NetworkManager/           # Network abstraction library
│   │   ├── INetworkManager.h     # Network manager interface
│   │   ├── ArduinoNetworkManager.h  # Arduino WiFi/OTA implementation
│   │   ├── ArduinoNetworkManager.cpp
│   │   ├── library.json
│   │   └── README.md
│   └── TaskManager/              # FreeRTOS task management
│       ├── TaskManager.h         # Task and mutex management
│       ├── TaskManager.cpp
│       ├── library.json
│       └── README.md
│
├── data/                         # Web interface files (HTML, CSS, JS)
│   ├── index.html
│   ├── js/
│   └── css/
│
├── scripts/                      # Build scripts
│   └── ccache.py                 # Compiler cache script
│
└── platformio.ini                # PlatformIO configuration

```

## Design Principles

### 1. Separation of Concerns
- **src/**: Contains only `.cpp` implementation files
- **include/**: Contains all `.h` header files organized by responsibility
- **lib/**: Contains reusable hardware drivers as independent libraries

### 2. Clear Module Boundaries
- **config/**: System-wide configuration (WiFi, pins, constants)
- **hardware/**: Hardware abstraction and device drivers
- **managers/**: Business logic and high-level system management
- **utilities/**: Helper functions and support utilities
- **lib/NetworkManager**: Network operations (WiFi, OTA) with framework abstraction
- **lib/TaskManager**: FreeRTOS task and synchronization management
- **lib/MAX31865Adapter**: Temperature sensor hardware driver

### 3. Include Path Conventions

Headers in include/ subdirectories:
```cpp
#include "config/Config.h"
#include "hardware/HeatingElement.h"
#include "managers/WebServerManager.h"
#include "utilities/SerialRemote.h"
```

Libraries in lib/:
```cpp
#include <MAX31865Adapter.h>
```

External dependencies:
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
```

### 4. PlatformIO Library Discovery
The project uses PlatformIO's automatic library discovery. Headers in `lib/` are included with angle brackets and discovered automatically.

### 5. Build Configuration
- `build_src_filter` in `platformio.ini` excludes deprecated code in `src/NotUsed/`
- `-I include` flag adds the include directory to the compiler search path
- Libraries in `lib/` are automatically discovered and linked

## Benefits of This Structure

1. **Maintainability**: Clear separation makes code easier to navigate and understand
2. **Scalability**: Easy to add new modules following existing patterns
3. **Testability**: Hardware abstraction through interfaces enables unit testing
4. **Reusability**: Libraries in `lib/` can be used in other projects
5. **Build Efficiency**: PlatformIO's LDF optimizes compilation and linking
6. **Standards Compliance**: Follows PlatformIO and C++ project conventions

## Thread Safety

The system uses FreeRTOS tasks for concurrent operation. Thread safety is ensured through mutex protection of shared state. See [THREAD_SAFETY.md](THREAD_SAFETY.md) for detailed documentation on:
- Lock hierarchy and rules
- Protected resources and operations
- Stress testing recommendations
- Debugging guidelines

## Migration Notes

Previous structure had:
- Headers mixed with `.cpp` files in `src/`
- Manager files in `src/Managers/` subfolder
- Generic names (Explorer instead of FileSystemExplorer)
- Hardware drivers in `src/` instead of `lib/`

This has been refactored to follow PlatformIO best practices.
