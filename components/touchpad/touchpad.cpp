
#include "esp_err.h"
#include "esp_intr.h"
#include "esp_log.h"

#include "driver/rtc_io.h"
#include "soc/rtc.h"

#include "touchpad.h"

//static IRAM_ATTR void isr_callback(void *arg);

TouchPad::TouchPad()
{
    initialized = false;
    callback = NULL;
    enabled = true;
}

TouchPad::~TouchPad()
{
    deinit();
}

// Initialize the touchpad module
esp_err_t TouchPad::init(Callback callback)
{
    esp_err_t err;

    // Initialize the touch pads
    err = touch_pad_init();
    if (err == ESP_OK)
    {
        // Set the interrupt handler
        err = touch_pad_isr_register(isr_callback, this);
        if (err == ESP_OK)
        {
            // And enable interrupts
            err = touch_pad_intr_enable();
            if (err == ESP_OK)
            {
                err = touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
                if (err == ESP_OK)
                {
                    // Initialize the pad configurations
                    for (uint8_t i = 0; i < TOUCH_PAD_MAX; ++i)
                    {
                        state[i].triggered = 0;
    
                        err = touch_pad_config(pads[i].chan, 0);
                        if (err != ESP_OK)
                        {
                            break;
                        } 

                        err = set_pad(i, 0, 0, 0, 0);
                        if (err != ESP_OK)
                        {
                            break;
                        } 
                    }

                    if (err == ESP_OK)
                    {
                        this->callback = callback;
                        initialized = true;
                        return ESP_OK;
                    }
                }
                touch_pad_intr_disable();
            }
            touch_pad_isr_deregister(isr_callback, this);
        }
        touch_pad_deinit();
    }

    return err;
}

// Stop using the touchpads
esp_err_t TouchPad::deinit()
{
    // All done with the touch pads
    if (initialized)
    {
        touch_pad_intr_disable();
        touch_pad_isr_deregister(isr_callback, this);
        touch_pad_deinit();
        initialized = false;
    }

    return ESP_OK;
}

void TouchPad::enable()
{
    enabled = true;
}

void TouchPad::disable()
{
    enabled = false;
}

// Configure the desired pads
esp_err_t TouchPad::configure(const Config (&config)[TOUCH_PAD_MAX])
{
    esp_err_t err;

    for (int i = 0; i < TOUCH_PAD_MAX; ++i)
    {
        if (config[i].touched > 0)
        {
            err = set_pad(i, config[i]);
        }
        else
        {
            err = rtc_gpio_deinit(pads[i].gpio);
        }

        if (err != ESP_OK)
        {
            break;
        }
    }

    return err;
}

// Returns true if the given pad index (0-9) was triggered
bool TouchPad::is_triggered(uint8_t pad)
{
    return state[pad].triggered > 0;
}

// Set the pad configuration
esp_err_t TouchPad::set_pad(uint8_t pad, Config cfg)
{
    config[pad] = cfg;

    return reset_pad(pad);
}

// Set the pad configuration
esp_err_t TouchPad::set_pad(uint8_t pad,
                            uint32_t delay,
                            uint32_t repeat,
                            uint32_t touched,
                            uint32_t released)
{
    config[pad].delay = delay;
    config[pad].repeat = repeat;
    config[pad].touched = touched;
    config[pad].released = released;

    return reset_pad(pad);
}

esp_err_t TouchPad::set_delay(uint8_t pad, uint32_t delay)
{
    config[pad].delay = delay;

    return ESP_OK;
}

esp_err_t TouchPad::set_repeat(uint8_t pad, uint32_t repeat)
{
    config[pad].repeat = repeat;

    return ESP_OK;
}

esp_err_t TouchPad::set_touched(uint8_t pad, uint16_t touched)
{
    esp_err_t err;

    uint16_t thresh;
    err = touch_pad_get_thresh(pads[pad].chan, &thresh);
    if (err == ESP_OK)
    {
        if (thresh == config[pad].touched)
        {
            err = set_threshold(pad, touched);
            if (err == ESP_OK)
            {
                config[pad].touched = touched;
            }
        }
        else
        {
            config[pad].touched = touched;
        }
    }

    return err;
}

esp_err_t TouchPad::set_released(uint8_t pad, uint16_t released)
{
    esp_err_t err;

    uint16_t thresh;
    err = touch_pad_get_thresh(pads[pad].chan, &thresh);
    if (err == ESP_OK)
    {
        config[pad].released = released;
    }

    return err;
}

esp_err_t TouchPad::set_threshold(uint8_t pad, uint16_t threshold)
{
    return touch_pad_set_thresh(pads[pad].chan, threshold); //config[pad].touched);
}

esp_err_t TouchPad::reset_pad(uint8_t pad)
{
    esp_err_t err;

    err = set_threshold(pad, config[pad].touched);

    state[pad].firstTouched = 0;
    state[pad].lastTouched = 0;

    return err;
}

void TouchPad::isr()
{
    esp_err_t err;

    uint16_t vals[TOUCH_PAD_MAX];

    uint32_t pad_intr = touch_pad_get_status();
    for (int i = 0; i < TOUCH_PAD_MAX; ++i)
    {
        // Continue to the next pad if this one didn't generate the interrupt
        if (((pad_intr >> i) & 0x01))
        {
            // Get the measurement value
            touch_pad_read(pads[i].chan, &vals[i]);
        }
        else
        {
            vals[i] = USHRT_MAX;
        }
    }
    touch_pad_clear_status();

    uint32_t ms = xTaskGetTickCountFromISR() / portTICK_PERIOD_MS;
    for (int i = 0; i < TOUCH_PAD_MAX; ++i)
    {
        if (vals[i] == USHRT_MAX)
        {
            continue;
        }

#if 1
ets_printf("i = %d, channel = %d, gpio = %d, val = %d, ms %d, first %d, last %d, triggered %d\n",
        i,
        pads[i].chan,
        pads[i].gpio,
        vals[i],
        ms,
        state[i].firstTouched,
        state[i].lastTouched,
        state[i].triggered);
#endif

        // Don't send an event unless touched
        TouchType type = TouchType::none;

        // Remember the ms
        state[i].lastTouched = ms;

        // User must have stopped touching the pad
        if (vals[i] > config[i].released)
        {
            err = reset_pad(i);
            if (err != ESP_OK)
            {
                return;
            }

            if (state[i].triggered)
            {
                state[i].triggered = 0;
                type = TouchType::release;
            }
        }
        // Otherwise, a new or continued touch was detected
        else if (config[i].repeat || state[i].triggered == 0)
        {
            if (state[i].triggered)
            {
                if (config[i].repeat && ms - state[i].triggered >= config[i].repeat)
                {
                    state[i].triggered = ms;
                    type = TouchType::repeat;
                }
            }
            else
            {
                // No delay, notify immediately
                if (config[i].delay == 0)
                {
                    state[i].triggered = ms;
                    type = TouchType::touch;
                }
                // This is a new touch, so record when it happened
                else if (state[i].firstTouched == 0)
                {
                    state[i].firstTouched = ms;
                }
                // Pad has been touched for the desired ms
                else if (ms - state[i].firstTouched >= config[i].delay)
                {
                    state[i].triggered = ms;
                    type = TouchType::touch;
                }
            }

            // Start looking for the release threshold.
            err = set_threshold(i, USHRT_MAX);
            if (err != ESP_OK)
            {
                return;
            }
        }

        // Send the nofication if needed
        if (type != TouchType::none && enabled)
        {
            callback(i, type, ms);
        }
    }
}

void TouchPad::isr_callback(void *arg)
{
    static_cast<TouchPad *>(arg)->isr();
}

