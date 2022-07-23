# hx711-pico-c

This is my implementation of reading from a HX711 via a Raspberry Pi Pico. It uses the RP2040's PIO feature to be as efficient as possible.

NOTE: if you are looking for a method to weigh objects, see [pico-scale](https://github.com/endail/pico-scale).

## Clone Repository

```console
git clone https://github.com/endail/hx711-pico-c
```

After building, copy `main.uf2` in the build directory to the Raspberry Pi Pico and then open up a serial connection to the Pico at a baud rate of 115200.

I have used [this helpful tutorial](https://paulbupejr.com/raspberry-pi-pico-windows-development/) to setup my Windows environment in order to program the Pico via Visual Studio Code.

![resources/hx711_serialout.gif](resources/hx711_serialout.gif)

The .gif above illustrates the [current example code](main.c) obtaining data from a HX711 operating at 80 samples per second.

## How to Use

See [here](https://learn.adafruit.com/assets/99339) for a pinout to choose GPIO pins.

```c
#include "include/hx711.h"
#include "include/hx711_noblock.pio.h" // for hx711_noblock_program and hx711_noblock_program_init

hx711_t hx;

// 1. Initialise the HX711
hx711_init(
    &hx,
    clkPin, // GPIO pin
    datPin, // GPIO pin
    pio0, // the RP2040 PIO to use
    &hx711_noblock_program, // the state machine program
    &hx711_noblock_program_init); // the state machine program init function

// 2. Power up
hx711_set_power(&hx, hx711_pwr_up);

// 3. [OPTIONAL] set gain and save it to the HX711
// chip by powering down then back up
hx711_set_gain(&hx, hx711_gain_128);
hx711_set_power(&hx, hx711_pwr_down);
hx711_wait_power_down();
hx711_set_power(&hx, hx711_pwr_up);

// 4. Wait for readings to settle
hx711_wait_settle(hx711_rate_10); // or hx711_rate_80 depending on your chip's config

// 5. Read values
int32_t val;

// block until a value is read
val = hx711_get_value(&hx);

// or use a timeout
// #include "pico/time.h" to use make_timeout_time_ms and make_timeout_time_us functions
absolute_time_t timeout = make_timeout_time_ms(250);

bool ok = hx711_get_value_timeout(
    &hx,
    &timeout,
    &val);

if(ok) {
    // value was obtained within the timeout period
    printf("%li\n", val);
}
```
