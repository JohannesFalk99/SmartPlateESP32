# TaskManager Library

## Overview
The TaskManager library provides centralized FreeRTOS task creation and synchronization management for the SmartPlate firmware.

## Features

### Task Management
- **Centralized Task Creation**: Single point for creating and tracking FreeRTOS tasks
- **Task Lifecycle**: Automatic tracking and cleanup of created tasks
- **Core Pinning**: Support for CPU core affinity
- **Error Handling**: Checks for task creation failures

### Synchronization
- **Mutex Management**: Create and manage FreeRTOS mutexes
- **Safe Operations**: Wrapper methods for mutex take/give operations
- **Timeout Support**: Configurable timeout for mutex acquisition

## Benefits

1. **Reduced Boilerplate**: Simplifies task and mutex creation
2. **Consistent Patterns**: Enforces consistent usage across codebase
3. **Debugging**: Centralized logging of task operations
4. **Resource Tracking**: Keeps track of all managed tasks and mutexes
5. **Safety**: Prevents common FreeRTOS pitfalls

## Usage

### Creating Tasks
```cpp
#include <TaskManager.h>

TaskManager taskManager;

void myTask(void* parameter) {
    while (true) {
        // Task code here
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// In setup():
TaskHandle_t handle = taskManager.createTask(
    "MyTask",      // Task name
    myTask,        // Task function
    NULL,          // Parameter
    4096,          // Stack size
    1,             // Priority
    1              // Core ID
);
```

### Using Mutexes
```cpp
// Create mutex
SemaphoreHandle_t mutex = taskManager.createMutex();

// In task:
if (taskManager.takeMutex(mutex, 100)) {
    // Critical section
    taskManager.giveMutex(mutex);
}
```

## Design Decisions

### Why Centralize Task Management?
1. **Visibility**: Easy to see all tasks at a glance
2. **Debugging**: Centralized logging helps track task lifecycle
3. **Resource Limits**: Prevents exceeding system task limits
4. **Cleanup**: Ensures proper cleanup on shutdown

### Thread Safety
- TaskManager itself is not thread-safe for task creation/deletion
- Should be used primarily from main setup() context
- Mutex operations are thread-safe (use FreeRTOS primitives)

## Limitations

- Maximum 10 tasks tracked (configurable via MAX_TASKS constant)
- Task deletion is simplified (doesn't handle all edge cases)
- No task suspension/resume functionality yet

## Future Enhancements

- Task statistics and monitoring
- Queue management
- Event group management  
- Task watchdog integration
- Dynamic task priority adjustment
