#if !defined(_TOUCHPAD_H_)
#define _TOUCHPAD_H_ 1

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_err.h"

#include "driver/touch_pad.h"

class TouchPad
{
public:

    class Config
    {
    public:
        uint32_t delay;     // # of milliseconds pad must be touched before notify, 0 = immediate
        uint32_t repeat;    // # of milliseconds between notifies if pad is held, 0 = no repeat
        uint16_t touched;   // touch detection threshold
        uint16_t released;  // release detection threshold
    };

    // Touch event types
    typedef enum
    {
        none,
        touch,
        repeat,
        release,
    } TouchType;

    // Event callback definition
    typedef void (*Callback)(uint8_t pad, TouchType type, int64_t ms);

    TouchPad();
    virtual ~TouchPad();

    // Initialize the touchpad module
    esp_err_t init(Callback callback);

    // Deinittialize the touchpad module
    esp_err_t deinit();

    // Disable touch detection.
    void disable();

    // Enable touch detection.
    void enable();

    // Configure the desired pads
    esp_err_t configure(const Config (&config)[TOUCH_PAD_MAX]);

    // Returns true if the given pad index (0-9) was triggered
    bool is_triggered(uint8_t pad);

    // Set the pad configuration
    esp_err_t set_pad(uint8_t pad, Config cfg);

    // Set the pad configuration
    esp_err_t set_pad(uint8_t pad,
                      uint32_t delay,
                      uint32_t repeat,
                      uint32_t touched,
                      uint32_t released);

    // Set the pad "delay" time
    esp_err_t set_delay(uint8_t pad, uint32_t delay);

    // Set the pad "repeat" time
    esp_err_t set_repeat(uint8_t pad, uint32_t repeat);

    // Set the pad "touched" threshold
    esp_err_t set_touched(uint8_t pad, uint16_t touched);

    // Set the pad "released" threshold
    esp_err_t set_released(uint8_t pad, uint16_t released);

private:
    esp_err_t set_threshold(uint8_t pad, uint16_t threshold);
    esp_err_t reset_pad(uint8_t pad);

    void isr();
    static void isr_callback(void *arg);

private:
    bool initialized;
    bool enabled;

    Callback callback;

    Config config[TOUCH_PAD_MAX];

    struct
    {
        uint32_t firstTouched;
        uint32_t lastTouched;
        uint32_t triggered;
    } state[TOUCH_PAD_MAX];

    const struct
    {
        touch_pad_t chan;
        gpio_num_t gpio;
    }
    pads[TOUCH_PAD_MAX] =
    {
        { TOUCH_PAD_GPIO4_CHANNEL,  (gpio_num_t) TOUCH_PAD_NUM0_GPIO_NUM },
        { TOUCH_PAD_GPIO0_CHANNEL,  (gpio_num_t) TOUCH_PAD_NUM1_GPIO_NUM },
        { TOUCH_PAD_GPIO2_CHANNEL,  (gpio_num_t) TOUCH_PAD_NUM2_GPIO_NUM },
        { TOUCH_PAD_GPIO15_CHANNEL, (gpio_num_t) TOUCH_PAD_NUM3_GPIO_NUM },
        { TOUCH_PAD_GPIO13_CHANNEL, (gpio_num_t) TOUCH_PAD_NUM4_GPIO_NUM },
        { TOUCH_PAD_GPIO12_CHANNEL, (gpio_num_t) TOUCH_PAD_NUM5_GPIO_NUM },
        { TOUCH_PAD_GPIO14_CHANNEL, (gpio_num_t) TOUCH_PAD_NUM6_GPIO_NUM },
        { TOUCH_PAD_GPIO27_CHANNEL, (gpio_num_t) TOUCH_PAD_NUM7_GPIO_NUM },
        { TOUCH_PAD_GPIO33_CHANNEL, (gpio_num_t) TOUCH_PAD_NUM8_GPIO_NUM },
        { TOUCH_PAD_GPIO32_CHANNEL, (gpio_num_t) TOUCH_PAD_NUM9_GPIO_NUM }
    };
};

#endif
