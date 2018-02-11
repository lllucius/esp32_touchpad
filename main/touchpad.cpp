
#include "touchpad.h"

static const TouchPad::Config pad_config[TOUCH_PAD_MAX] =
{
//    --- event ---  --- threshold ---
//    delay  repeat  touched  released
    {    3,      0,     200,       800 },   // TOUCH_PAD_NUM0 - GPIO4
    {    0,      0,       0,         0 },   // TOUCH_PAD_NUM1 - GPIO0
    {    3,     10,     200,       800 },   // TOUCH_PAD_NUM2 - GPIO2
    {    3,     10,     200,       800 },   // TOUCH_PAD_NUM3 - GPIO15
    {    0,      0,       0,         0 },   // TOUCH_PAD_NUM4 - GPIO13
    {    0,     10,     200,       800 },   // TOUCH_PAD_NUM5 - GPIO12
    {    3,     10,     200,       800 },   // TOUCH_PAD_NUM6 - GPIO14
    {    3,     10,     200,       800 },   // TOUCH_PAD_NUM7 - GPIO27
    {    3,     10,     200,       800 },   // TOUCH_PAD_NUM8 - GPIO33
    {    3,     10,     200,       800 },   // TOUCH_PAD_NUM9 - GPIO32
};

TouchPad touchpad;
QueueHandle_t event_queue;

typedef struct
{
    int64_t ms;
    int pad;
    TouchPad::TouchType type;
} event_t;

// Invoked from ISR
static void touch_cb(uint8_t pad, TouchPad::TouchType type, int64_t ms)
{
    event_t evt = {};
    evt.ms = ms;
    evt.pad = pad;
    evt.type = type;

    BaseType_t wokeup;
    if (xQueueSendFromISR(event_queue, &evt, &wokeup) == pdTRUE)
    {
        if (wokeup)
        {
            portYIELD_FROM_ISR();
        }
    }
    else
    {
        ets_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        ets_printf("touch_cb: xQueueSendFromISR failed\n");
        ets_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    }
}

extern "C" void app_main()
{
    printf("Test ESP32 touch pads\n");

    // Create the queue where we'll receive our events
    event_queue = xQueueCreate(16, sizeof(event_t));
    if (event_queue == NULL)
    {
        printf("Faile to create queue\n");
        abort();
    }

    // Configure the touch pads
    touchpad.init(touch_cb);
    touchpad.configure(pad_config);

    for (;;)
    {
        event_t evt;

        xQueueReceive(event_queue, &evt, portMAX_DELAY);

        printf("touch pad %d triggered %d\n", evt.pad, evt.type);
    }

    // Never reached
}

