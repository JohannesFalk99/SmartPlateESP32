# Thread Safety Documentation

## Overview

This document describes the thread-safety mechanisms implemented in SmartPlate ESP32 to prevent race conditions when accessing shared state from multiple FreeRTOS tasks.

## System Architecture

### Tasks

The system runs three FreeRTOS tasks on ESP32 Core 1:

1. **heaterTask** (Priority 1)
   - Runs every 500ms
   - Updates heater control logic via `heater.update()`
   - Can trigger callbacks that access state (e.g., `temperatureChanged()`)

2. **webTask** (Priority 1)
   - Runs every 50ms
   - Processes WebSocket messages via `WebServerManager::handle()`
   - Can modify state via web interface commands

3. **stateTask** (Priority 1)
   - Runs every 100ms
   - Updates system state with sensor readings
   - Notifies web clients of state changes
   - Updates mode manager

### Shared Resources

The following shared resources are protected by `stateMutex`:

- **SystemState state** - Global system state structure containing:
  - `temperature` - Current temperature reading
  - `rpm` - Current RPM value
  - `mode` - Current operating mode
  - `tempSetpoint` - Temperature setpoint
  - `rpmSetpoint` - RPM setpoint
  - `duration` - Duration setting
  - `alertTempThreshold`, `alertRpmThreshold`, `alertTimerThreshold` - Alert thresholds
  - `startTime` - System start time

- **history[]** - Circular buffer of temperature history entries (300 entries)
- **historyIndex** - Current write position in history buffer

## Synchronization Mechanism

### Mutex: stateMutex

A single FreeRTOS mutex (`SemaphoreHandle_t stateMutex`) protects all shared state access.

**Creation:**
```cpp
stateMutex = xSemaphoreCreateMutex();  // In setup() before tasks start
```

**Lock Acquisition:**
```cpp
if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // Critical section - access shared state
    xSemaphoreGive(stateMutex);
}
```

**Timeout:** All lock acquisitions use a 100ms timeout to prevent deadlocks

**Graceful Degradation:** If lock cannot be acquired within timeout, operations are skipped or use cached values

## Lock Hierarchy and Rules

### Single-Level Hierarchy

There is only one mutex in the system, so no lock ordering concerns exist. However, the following rules apply:

1. **Minimize Lock Hold Time**
   - Acquire lock only for accessing/modifying shared state
   - Release lock before performing I/O operations (network, serial, file system)
   - Never call blocking operations while holding the lock

2. **No Nested Locking**
   - Functions should not call other functions that acquire the same lock
   - Use helper functions that assume lock is already held (documented with comments)

3. **Consistent Timeout**
   - Always use 100ms timeout: `pdMS_TO_TICKS(100)`
   - Ensures bounded waiting time

## Protected Operations by Module

### StateManager (src/StateManager.cpp)

**Functions:**
- `updateState()` - Acquires lock, updates state, releases lock
- `logState()` - Acquires lock, reads state for logging, releases lock

**Lock Pattern:**
```cpp
bool haveLock = false;
if (mutex && xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    haveLock = true;
}
// Access state
if (haveLock) {
    xSemaphoreGive(mutex);
}
```

### WebServerManager (src/WebServerManager.cpp)

**Protected Methods:**

1. **notifyClients()**
   - Acquires lock to read state
   - Releases lock BEFORE network send operation
   - Calls `StateManager::logState()` with mutex

2. **handleControlUpdate()**
   - Acquires lock to read current state values
   - Releases lock before calling `StateManager::updateState()`
   - Re-acquires lock to update `startTime`
   - Releases lock before calling `notifyClients()`

3. **addHistoryEntry()**
   - Acquires lock
   - Modifies history buffer and index
   - Releases lock

4. **handleGetHistory()**
   - Acquires lock to read entire history buffer
   - Releases lock before network serialization and send

5. **Mode Handlers (handleModeHold, handleModeRamp, handleModeTimer)**
   - Acquire lock to read setpoint values
   - Release lock before calling mode manager methods

6. **Action Handlers (getConfig, updateState lambdas)**
   - Acquire lock for state reads/writes
   - Release lock before network operations

### main.cpp

**stateTask:**
- Acquires lock at start of each iteration
- Calls `updateRPM()`, `updateSystemState()`, `notifyClients()`, `modeManager.update()`
- Releases lock at end of iteration
- All state modifications happen within this protected region

**temperatureChanged() callback:**
- Calls `WebServerManager::addHistoryEntry()` which has its own mutex protection
- No direct state access

## Lock Coverage Analysis

### Protected Paths

✅ **Web Interface → State Modification**
- WebSocket message → `handleControlUpdate()` → `StateManager::updateState()` → Protected

✅ **Periodic State Updates**
- `stateTask` → `updateSystemState()` → Direct state write → Protected by task's lock

✅ **State Broadcasting**
- `stateTask` → `notifyClients()` → State read → Protected

✅ **History Updates**
- Temperature callback → `addHistoryEntry()` → History write → Protected

✅ **Configuration Queries**
- WebSocket request → `getConfig` handler → State read → Protected

### Design Considerations

**Why Release Lock Before Network I/O?**
- Network operations (WebSocket send, TCP send) can be slow
- Holding lock during I/O would block other tasks unnecessarily
- State is copied to JSON document, then lock is released before sending

**Why 100ms Timeout?**
- Tasks run every 50-500ms, so 100ms is reasonable upper bound
- Prevents indefinite blocking if something goes wrong
- Allows graceful degradation (skip update if lock unavailable)

**Why Single Mutex?**
- Simple reasoning about correctness
- No risk of deadlock from lock ordering
- Shared state is small and access patterns are simple
- Fine-grained locking would add complexity without meaningful benefit

## Stress Testing Recommendations

To validate thread safety under load:

1. **Rapid WebSocket Commands**
   - Send control updates at high frequency (>10/sec)
   - Verify state remains consistent
   - Check for dropped messages or timeouts

2. **Concurrent State Queries**
   - Multiple clients requesting history simultaneously
   - Verify data integrity and no crashes

3. **Mixed Operations**
   - Simultaneous reads (getHistory, getConfig) and writes (controlUpdate)
   - Verify no torn reads (partial old/new state)

4. **Lock Acquisition Failures**
   - Monitor logs for failed lock acquisitions
   - Should be rare under normal load
   - If frequent, may indicate lock held too long

## Known Limitations

1. **No Read-Write Lock**
   - Single mutex means readers block each other
   - For this application, read/write contention is low enough that this is acceptable

2. **No Priority Inheritance**
   - FreeRTOS mutexes do support priority inheritance by default
   - All tasks are same priority, so not an issue

3. **No State Versioning**
   - If lock acquisition fails, operation uses whatever state it can read
   - No guarantee of consistent snapshot across multiple state fields
   - For this application, acceptable tradeoff

## Future Enhancements

If thread-safety issues persist or performance degrades:

1. Consider atomic types for simple flags/counters
2. Implement lock-free ring buffer for history
3. Add state versioning or sequence numbers
4. Split state into multiple independently-locked regions
5. Add deadlock detection and recovery

## Debugging

If encountering suspected race conditions:

1. Enable verbose logging of lock acquire/release
2. Add assertions to verify lock is held when accessing state
3. Use ESP-IDF tracing to track task execution
4. Monitor lock wait times and timeouts
5. Use JTAG debugger to inspect state during execution
