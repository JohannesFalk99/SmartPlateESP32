#pragma once
#include <functional>
#include <atomic>
#include "driver/gpio.h"
#include "esp_timer.h"

class StirrerPhase
{
public:
    using Callback = void (*)();

    // Constructor
    // zc_gpio: zero-cross input GPIO (H11AA1 output) - configured as input_pullup, active rising
    // moc_gpio: GPIO that drives MOC3021 LED (through series resistor)
    // mainsHz: 50 or 60
    StirrerPhase(gpio_num_t zc_gpio, gpio_num_t moc_gpio, int mainsHz = 50);

    ~StirrerPhase();

    // Call once after power-up
    void begin();

    // Call periodically if you want to run background checks (optional)
    void update();

    // Control
    void start();                 // allow firing
    void stop();                  // disable firing (motor off)
    void setTargetRPM(float rpm); // target rpm (open-loop mapping)
    float getTargetRPM() const;
    float getCurrentEstimate() const; // naive open-loop estimate

    bool isRunning() const;
    bool hasFault() const;

    // Callbacks
    void setOnStartCallback(Callback cb);
    void setOnStopCallback(Callback cb);
    void setOnReachedCallback(Callback cb);
    void setOnFaultCallback(Callback cb);
    void setOnSpeedChangedCallback(void (*cb)(float));

    // Parameters you may tweak
    void setGatePulseMicroseconds(uint32_t us); // default ~120
    void setMinPercent(float p);                // minimum % (to avoid too-small conduction)
    void setMaxPercent(float p);

private:
    // GPIOs
    gpio_num_t zcGpio;
    gpio_num_t mocGpio;
    int mainsHz;
    uint32_t halfCycleUs;

    // Timing / state
    std::atomic<uint32_t> delayUs; // microsecond delay after zero-cross to fire
    std::atomic<bool> fireScheduled;
    std::atomic<int64_t> lastZcUsec;

    // target / mapping
    std::atomic<float> targetRpm;
    std::atomic<float> currentEstimate;
    float minPercent = 5.0f;
    float maxPercent = 95.0f;
    float maxRpm = 3000.0f; // used for open-loop mapping; tune for your motor

    // gate pulse width
    uint32_t gatePulseUs = 120;

    // esp_timer handles (single-shot)
    esp_timer_handle_t delayTimer = nullptr;
    esp_timer_handle_t pulseTimer = nullptr;

    // Callbacks
    Callback onStart = nullptr;
    Callback onStop = nullptr;
    Callback onReached = nullptr;
    Callback onFault = nullptr;
    void (*onSpeedChanged)(float) = nullptr;

    // internal flags
    std::atomic<bool> running;
    std::atomic<bool> fault;
    std::atomic<bool> targetReachedTriggered;
    std::atomic<bool> begun;

private:
    // helpers
    void scheduleFireFromNow(uint32_t us);
    static void IRAM_ATTR delayTimerCb(void *arg);
    static void IRAM_ATTR pulseTimerCb(void *arg);

    // Zero-cross ISR (C function)
    static void IRAM_ATTR zc_isr_handler(void *arg);

    // mapping
    uint32_t computeDelayFromPercent(float pct) const;
    float rpmToPercent(float rpm) const;
};
