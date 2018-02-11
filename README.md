# ESP32 touch pad component
The TouchPad component provides a means of using the ESP32 touch pads
as switches.

Your callback is invoked when a touch is detected, at intervals if it's
a continuous touch, and when the touch is released.

You specify configuration options for each touch pad:
```
    class Config
    {
    public:
        uint32_t delay;     // # of milliseconds pad must be touched before notify, 0 = immediate
        uint32_t repeat;    // # of milliseconds between notifies if pad is held, 0 = no repeat
        uint16_t touched;   // touch detection threshold
        uint16_t released;  // release detection threshold
    };
```

To use, just add the "touchpad" component to your components directory
and initialize it with something like:

```
#include "touchpad.h"

static void touch_cb(uint8_t pad, TouchPad::TouchType type, int64_t ms)
{
    ...notify main task...
}

void app_main()
{
    TouchPad tp;

    touchpad.init(touch_cb);
    touchpad.configure(pad_config);

    for (;;)
    {
        ...wait for and process notifications...
    }
}
```

