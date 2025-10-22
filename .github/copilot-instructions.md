# GitHub Copilot Instructions for SmartPlate ESP32

## Project Overview

SmartPlate ESP32 is a PlatformIO-based ESP32 project for a smart heating plate controller with web interface, temperature sensing, and mode management. The project follows PlatformIO best practices with clear separation of concerns.

## Project Structure

The project uses a structured approach with clear boundaries:

```
SmartPlateESP32/
├── src/                          # Application source files (.cpp only)
├── include/                      # Public header files
│   ├── config/                   # System-wide configuration
│   ├── hardware/                 # Hardware abstraction
│   ├── managers/                 # Business logic
│   └── utilities/                # Helper functions
├── lib/                          # Project-specific libraries
│   └── MAX31865Adapter/          # Hardware driver library
├── data/                         # Web interface files
├── scripts/                      # Build scripts
└── platformio.ini                # PlatformIO configuration
```

### Key Design Principles

1. **Separation of Concerns**:
   - `src/` contains ONLY `.cpp` implementation files
   - `include/` contains ALL `.h` header files organized by responsibility
   - `lib/` contains reusable hardware drivers as independent libraries

2. **Module Boundaries**:
   - `config/`: System-wide configuration (WiFi, pins, constants)
   - `hardware/`: Hardware abstraction and device drivers
   - `managers/`: Business logic and high-level system management
   - `utilities/`: Helper functions and support utilities

3. **Flat Source Structure**: All `.cpp` files are directly in `src/` root (no nested folders)

## Coding Conventions

### Include Path Conventions

**Headers in include/ subdirectories** (use quotes):
```cpp
#include "config/Config.h"
#include "hardware/HeatingElement.h"
#include "managers/WebServerManager.h"
#include "utilities/SerialRemote.h"
```

**Libraries in lib/** (use angle brackets):
```cpp
#include <MAX31865Adapter.h>
```

**External dependencies** (use angle brackets):
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
```

### File Naming

- Use descriptive names (e.g., `FileSystemExplorer` not `Explorer`)
- Header files: PascalCase (e.g., `HeatingElement.h`)
- Implementation files: Match header name (e.g., `HeatingElement.cpp`)
- Organize by responsibility, not just file type

### Build Configuration

- The project uses PlatformIO with ESP32 Arduino framework
- `build_src_filter` in `platformio.ini` excludes deprecated code in `src/NotUsed/`
- The `-I include` flag adds the include directory to the compiler search path
- Libraries in `lib/` are automatically discovered and linked by PlatformIO

## When Adding New Code

### Adding a New Feature

1. **Determine the layer**: Is it hardware, business logic, or utility?
2. **Create header in appropriate include/ subdirectory**:
   - Hardware abstraction → `include/hardware/`
   - Business logic/managers → `include/managers/`
   - Helper utilities → `include/utilities/`
   - Configuration → `include/config/`
3. **Create implementation in src/** (flat, no subfolders)
4. **Use correct include syntax** based on location

### Adding a Hardware Driver

1. Create a new library in `lib/` with proper structure:
   ```
   lib/DriverName/
   ├── DriverName.h
   └── library.json
   ```
2. Include it with angle brackets: `#include <DriverName.h>`

### Modifying Existing Code

- **DO NOT** move `.cpp` files into subdirectories within `src/`
- **DO NOT** place `.h` files in `src/`
- **DO NOT** use nested paths for source files
- Follow existing patterns for include statements

## Common Patterns

### Hardware Abstraction

Use interfaces for hardware abstraction:
```cpp
// include/hardware/ITemperatureSensor.h
class ITemperatureSensor {
public:
    virtual float readTemperature() = 0;
    virtual ~ITemperatureSensor() = default;
};

// lib/MAX31865Adapter/MAX31865Adapter.h
class MAX31865Adapter : public ITemperatureSensor {
    // Implementation
};
```

### Manager Classes

Business logic managers follow a consistent pattern:
- Declared in `include/managers/`
- Implemented in `src/`
- Handle high-level system state and coordination

### Web Server Integration

- WebSocket actions are in `utilities/WebServerActions.h`
- Web server management is in `managers/WebServerManager.h`
- Static web files are in `data/`

## Build and Development

- Use PlatformIO CLI or IDE for building: `pio run`
- Upload to ESP32: `pio run --target upload`
- Monitor serial output: `pio device monitor`
- The project uses OTA updates (over-the-air) by default

## Documentation

- See `STRUCTURE.md` for detailed project structure documentation
- See `MIGRATION.md` for historical context on structure changes
- Update documentation when making structural changes

## Important Notes

- This project has been refactored to follow PlatformIO best practices
- The old structure had headers mixed with `.cpp` files—this is no longer the case
- Always maintain clear separation between headers and implementation
- When in doubt, follow the existing patterns in the codebase
