#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <functional>

/**
 * @brief Manages FreeRTOS tasks and inter-task communication
 * 
 * This class provides a centralized way to create, manage, and coordinate
 * FreeRTOS tasks with proper synchronization primitives.
 */
class TaskManager {
public:
    /**
     * @brief Task function signature
     */
    using TaskFunction = void (*)(void*);
    
    /**
     * @brief Construct a new Task Manager
     */
    TaskManager();
    
    /**
     * @brief Destroy the Task Manager and clean up resources
     */
    ~TaskManager();
    
    /**
     * @brief Create a new FreeRTOS task
     * @param name Task name for debugging
     * @param taskFunction Function to execute in task
     * @param parameter Parameter to pass to task function
     * @param stackSize Stack size in bytes
     * @param priority Task priority (0-configMAX_PRIORITIES-1)
     * @param coreId CPU core to pin task to (0, 1, or tskNO_AFFINITY)
     * @return TaskHandle_t Handle to created task, NULL on failure
     */
    TaskHandle_t createTask(
        const char* name,
        TaskFunction taskFunction,
        void* parameter,
        uint32_t stackSize,
        UBaseType_t priority,
        BaseType_t coreId
    );
    
    /**
     * @brief Delete a task
     * @param taskHandle Handle to task to delete
     */
    void deleteTask(TaskHandle_t taskHandle);
    
    /**
     * @brief Create a mutex for synchronization
     * @return SemaphoreHandle_t Handle to mutex, NULL on failure
     */
    SemaphoreHandle_t createMutex();
    
    /**
     * @brief Delete a mutex
     * @param mutex Handle to mutex to delete
     */
    void deleteMutex(SemaphoreHandle_t mutex);
    
    /**
     * @brief Take a mutex with timeout
     * @param mutex Mutex to take
     * @param timeoutMs Timeout in milliseconds
     * @return true if mutex acquired, false on timeout
     */
    bool takeMutex(SemaphoreHandle_t mutex, uint32_t timeoutMs);
    
    /**
     * @brief Give/release a mutex
     * @param mutex Mutex to release
     */
    void giveMutex(SemaphoreHandle_t mutex);
    
    /**
     * @brief Get number of created tasks
     * @return uint32_t Number of tasks managed by this instance
     */
    uint32_t getTaskCount() const { return taskCount; }

private:
    uint32_t taskCount = 0;  ///< Number of tasks created
    
    static const uint32_t MAX_TASKS = 10;  ///< Maximum number of tasks
    TaskHandle_t tasks[MAX_TASKS];  ///< Array of task handles
};
