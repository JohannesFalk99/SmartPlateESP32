#include "TaskManager.h"

TaskManager::TaskManager() : taskCount(0) {
    // Initialize task handle array
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        tasks[i] = NULL;
    }
}

TaskManager::~TaskManager() {
    // Clean up any remaining tasks
    for (uint32_t i = 0; i < taskCount && i < MAX_TASKS; i++) {
        if (tasks[i] != NULL) {
            vTaskDelete(tasks[i]);
        }
    }
}

TaskHandle_t TaskManager::createTask(
    const char* name,
    TaskFunction taskFunction,
    void* parameter,
    uint32_t stackSize,
    UBaseType_t priority,
    BaseType_t coreId
) {
    if (taskCount >= MAX_TASKS) {
        Serial.printf("[TaskManager] Error: Maximum task count (%d) reached\n", MAX_TASKS);
        return NULL;
    }
    
    TaskHandle_t taskHandle = NULL;
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        name,
        stackSize,
        parameter,
        priority,
        &taskHandle,
        coreId
    );
    
    if (result == pdPASS && taskHandle != NULL) {
        tasks[taskCount] = taskHandle;
        taskCount++;
        Serial.printf("[TaskManager] Created task '%s' (count: %d)\n", name, taskCount);
        return taskHandle;
    } else {
        Serial.printf("[TaskManager] Failed to create task '%s'\n", name);
        return NULL;
    }
}

void TaskManager::deleteTask(TaskHandle_t taskHandle) {
    if (taskHandle == NULL) return;
    
    // Find and remove from tracking array
    for (uint32_t i = 0; i < taskCount; i++) {
        if (tasks[i] == taskHandle) {
            vTaskDelete(taskHandle);
            // Shift remaining tasks down
            for (uint32_t j = i; j < taskCount - 1; j++) {
                tasks[j] = tasks[j + 1];
            }
            tasks[taskCount - 1] = NULL;
            taskCount--;
            Serial.printf("[TaskManager] Deleted task (count: %d)\n", taskCount);
            return;
        }
    }
}

SemaphoreHandle_t TaskManager::createMutex() {
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    if (mutex != NULL) {
        Serial.println("[TaskManager] Mutex created");
    } else {
        Serial.println("[TaskManager] Failed to create mutex");
    }
    return mutex;
}

void TaskManager::deleteMutex(SemaphoreHandle_t mutex) {
    if (mutex != NULL) {
        vSemaphoreDelete(mutex);
        Serial.println("[TaskManager] Mutex deleted");
    }
}

bool TaskManager::takeMutex(SemaphoreHandle_t mutex, uint32_t timeoutMs) {
    if (mutex == NULL) return false;
    return xSemaphoreTake(mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void TaskManager::giveMutex(SemaphoreHandle_t mutex) {
    if (mutex != NULL) {
        xSemaphoreGive(mutex);
    }
}
