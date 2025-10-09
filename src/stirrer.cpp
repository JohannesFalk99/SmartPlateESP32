#include "stirrer.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <cmath>

static const char *TAG = "StirrerPhase";

// Single global pointer used by ISR callbacks to access the instance
// (This implementation supports one instance; extend if you need multiple.)
// NOTE: Only one StirrerPhase instance is supported at a time due to the global pointer used for ISR access.
// If multiple stirrers are needed, refactor to support multiple instances.
static StirrerPhase *g_instance = nullptr;

StirrerPhase::StirrerPhase(gpio_num_t zc_gpio, gpio_num_t moc_gpio, int mainsHz_)
    : zcGpio(zc_gpio), mocGpio(moc_gpio), mainsHz(mainsHz_),
      delayUs(0), fireScheduled(false), lastZcUsec(0),
      targetRpm(0.0f), currentEstimate(NAN),
      running(false), fault(false), targetReachedTriggered(false), begun(false)
{
    if (mainsHz == 50)
        halfCycleUs = 1000000U / 100; // 10 000 us
    else
        halfCycleUs = 1000000U / 120; // for 60 Hz -> 8333us
}

StirrerPhase::~StirrerPhase()
{
    if (delayTimer)
    {
        esp_timer_stop(delayTimer);
        esp_timer_delete(delayTimer);
    }
    if (pulseTimer)
    {
        esp_timer_stop(pulseTimer);
        esp_timer_delete(pulseTimer);
    }
    if (g_instance == this)
        g_instance = nullptr;
}

void StirrerPhase::begin()
{
    if (begun)
        return;
    begun = true;
    // configure MOC output pin
    gpio_reset_pin(mocGpio);
    gpio_set_direction(mocGpio, GPIO_MODE_OUTPUT);
    gpio_set_level(mocGpio, 0);

    // configure ZC input
    gpio_reset_pin(zcGpio);
    gpio_set_direction(zcGpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(zcGpio, GPIO_PULLUP_ONLY);

    // install ISR service and attach handler
    static bool isr_service_installed = false;
    if (!isr_service_installed) {
        if (gpio_install_isr_service(0) == ESP_OK) {
            isr_service_installed = true;
        } else {
            ESP_LOGW(TAG, "ISR service already installed or failed to install");
        }
    }
    esp_err_t isr_add_result = gpio_isr_handler_add(zcGpio, zc_isr_handler, (void *)this);
    if (isr_add_result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler: %d", isr_add_result);
    }

    // create esp_timers (single-shot)
    esp_timer_create_args_t delay_args = {};
    delay_args.callback = &StirrerPhase::delayTimerCb;
    delay_args.arg = this;
    delay_args.name = "stir_delay";
    esp_timer_create(&delay_args, &delayTimer);

    esp_timer_create_args_t pulse_args = {};
    pulse_args.callback = &StirrerPhase::pulseTimerCb;
    pulse_args.arg = this;
    pulse_args.name = "stir_pulse";
    esp_timer_create(&pulse_args, &pulseTimer);

    // register global instance for ISR access
    g_instance = this;

    ESP_LOGI(TAG, "StirrerPhase started: ZC GPIO=%d, MOC GPIO=%d, halfCycleUs=%u",
             zcGpio, mocGpio, halfCycleUs);
}

void StirrerPhase::update()
{
    // In open-loop mode, just update a naive currentEstimate for callbacks
    float prev = currentEstimate.load();
    float t = targetRpm.load();
    currentEstimate.store(t); // simple pass-through. Replace with tachometer logic if available
    if (onSpeedChanged && (std::isnan(prev) || fabs(prev - t) > 0.5f))
    {
        onSpeedChanged(t);
    }
    // fault checks could be added here (overtemp, missing zc, etc.)
}

void StirrerPhase::start()
{
    if (!begun)
        begin();
    if (!running && !fault)
    {
        running.store(true);
        if (onStart)
            onStart();
        ESP_LOGI(TAG, "StirrerPhase started");
    }
}

void StirrerPhase::stop()
{
    if (running)
    {
        running.store(false);
        // ensure MOC is off
        gpio_set_level(mocGpio, 0);
        if (onStop)
            onStop();
        ESP_LOGI(TAG, "StirrerPhase stopped");
    }
}

void StirrerPhase::setTargetRPM(float rpm)
{
    targetRpm.store(rpm);
    // compute percent mapping (open-loop)
    float pct = rpmToPercent(rpm);
    // clamp
    if (pct < minPercent)
        pct = minPercent;
    if (pct > maxPercent)
        pct = maxPercent;
    uint32_t d = computeDelayFromPercent(pct);
    delayUs.store(d);
    ESP_LOGI(TAG, "TargetRPM %.1f -> pct %.1f -> delay %u us", rpm, pct, d);
}

float StirrerPhase::getTargetRPM() const { return targetRpm.load(); }
float StirrerPhase::getCurrentEstimate() const { return currentEstimate.load(); }
bool StirrerPhase::isRunning() const { return running.load(); }
bool StirrerPhase::hasFault() const { return fault.load(); }

void StirrerPhase::setOnStartCallback(Callback cb) { onStart = cb; }
void StirrerPhase::setOnStopCallback(Callback cb) { onStop = cb; }
void StirrerPhase::setOnReachedCallback(Callback cb) { onReached = cb; }
void StirrerPhase::setOnFaultCallback(Callback cb) { onFault = cb; }
void StirrerPhase::setOnSpeedChangedCallback(void (*cb)(float)) { onSpeedChanged = cb; }

void StirrerPhase::setGatePulseMicroseconds(uint32_t us) { gatePulseUs = us; }
void StirrerPhase::setMinPercent(float p) { minPercent = p; }
void StirrerPhase::setMaxPercent(float p) { maxPercent = p; }

//
// Internal helper: schedule immediate single-shot after 'us' microseconds
//
void StirrerPhase::scheduleFireFromNow(uint32_t us)
{
    if (!running.load())
        return;
    if (us >= halfCycleUs)
        return; // guard
    // stop previous timer if any
    esp_timer_stop(delayTimer);
    esp_timer_start_once(delayTimer, us);
    fireScheduled.store(true);
}

//
// Static ISR handler called on zero-cross.
// We compute the delayUs value (already written by setTargetRPM) and arm the delay timer.
//
void IRAM_ATTR StirrerPhase::zc_isr_handler(void *arg)
{
    // Retrieve instance (we used gpio_isr_handler_add with this arg)
    StirrerPhase *inst = static_cast<StirrerPhase *>(arg);
    int64_t now = esp_timer_get_time();
    // debounce: ignore if very close to previous
    int64_t prev = inst->lastZcUsec.exchange(now);
    if (prev != 0 && (now - prev) < (inst->halfCycleUs / 3))
    {
        // spurious bounce - ignore
        return;
    }

    // read delayUs (atomic)
    uint32_t d = inst->delayUs.load();
    // schedule fire (single-shot) after delay d (us)
    inst->scheduleFireFromNow(d);
}

//
// delayTimer callback: actually fire the gate (set MOC pin high) and arm pulse off timer
//
void IRAM_ATTR StirrerPhase::delayTimerCb(void *arg)
{
    StirrerPhase *inst = static_cast<StirrerPhase *>(arg);
    // quick guard
    if (!inst->fireScheduled.load())
        return;

    // Drive MOC LED (GPIO) high to trigger optotriac. Must toggle on IRAM-safe functions
    gpio_set_level(inst->mocGpio, 1);

    // schedule pulse off
    esp_timer_start_once(inst->pulseTimer, inst->gatePulseUs);

    inst->fireScheduled.store(false);
}

//
// pulseTimer callback: turn off gate (MOC LED low)
//
void IRAM_ATTR StirrerPhase::pulseTimerCb(void *arg)
{
    StirrerPhase *inst = static_cast<StirrerPhase *>(arg);
    gpio_set_level(inst->mocGpio, 0);
}

//
// Utility: map percent 0..100 -> delay in us (0..halfCycleUs).
// Higher percent = smaller delay (earlier firing).
//
uint32_t StirrerPhase::computeDelayFromPercent(float pct) const
{
    // pct in [minPercent..maxPercent]
    float p = pct;
    if (p <= minPercent)
        p = minPercent;
    if (p >= maxPercent)
        p = maxPercent;
    // Normalize 0..1
    float x = (p - minPercent) / (maxPercent - minPercent); // 0..1
    // Use gamma mapping to feel more linear in torque
    const float gamma = 2.0f;
    float powerFrac = powf(x, gamma);   // 0..1
    float alphaFrac = 1.0f - powerFrac; // 0..1 firing-angle fraction
    uint32_t d = (uint32_t)(alphaFrac * (float)halfCycleUs);
    // small safety margin
    if (d > halfCycleUs - 200)
        d = halfCycleUs - 200;
    return d;
}

float StirrerPhase::rpmToPercent(float rpm) const
{
    // simple linear mapping to maxRpm
    float pct = (rpm / maxRpm) * 100.0f;
    return pct;
}
