# hx711-pico-c

This is my implementation of reading from a HX711 via a Raspberry Pi Pico. It uses the RP2040's PIO feature to be as efficient as possible.

__NOTE__: if you are looking for a method to weigh objects (ie. by using the HX711 as a scale), see [pico-scale](https://github.com/endail/pico-scale).

![resources/hx711_serialout.gif](resources/hx711_serialout.gif)

The .gif above illustrates the [current example code](main.c) obtaining data from a HX711 operating at 80 samples per second.

## Clone Repository

```console
git clone https://github.com/endail/hx711-pico-c
```

Alternatively, include it as a submodule in your project and then `#include "extern/hx711-pico-c/include/hx711.h"` and `#include "extern/hx711-pico-c/include/hx711_noblock.pio.h"`.

```console
git submodule add https://github.com/endail/hx711-pico-c extern/
git submodle update --init
```

## Documentation

[https://endail.github.io/hx711-pico-c](https://endail.github.io/hx711-pico-c/hx711_8h.html)

## How to Use

See [here](https://learn.adafruit.com/assets/99339) for a pinout to choose two GPIO pins on the Pico (RP2040). One GPIO pin to connect to the HX711's clock pin and a second GPIO pin to connect to the HX711's data pin. You can choose any two pins clock and data pins, as long as they are capable of digital output and input respectively.

```c
#include "include/hx711.h"
#include "include/hx711_noblock.pio.h" // for hx711_noblock_program and hx711_noblock_program_init

hx711_t hx;

// 1. Initialise the HX711
hx711_init(
    &hx,
    clkPin, // Pico GPIO pin connected to HX711's clock pin
    datPin, // Pico GPIO pin connected to HX711's data pin
    pio0, // the RP2040 PIO to use (either pio0 or pio1)
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

// wait (block) until a value is read
val = hx711_get_value(&hx);

// or use a timeout
// #include "pico/time.h" to use make_timeout_time_ms and make_timeout_time_us functions
absolute_time_t timeout = make_timeout_time_ms(250);

if(hx711_get_value_timeout(&hx, &timeout, &val)) {
    // value was obtained within the timeout period
    // in this example, within 250ms
    printf("%li\n", val);
}
```

## Custom PIO Programs

You will notice in the code example above that you need to manually include the `hx711_noblock.pio.h` PIO header file. This is because it is not included by default in the `hx711-pico-c` library. It is offered as _a method_ for reading from the HX711 that I have optimised as much as possible to run as efficiently as possible. But there is nothing stopping you from creating your own PIO program and using it with the `hx711_t`. In fact, if you do want to make your own HX711 PIO program, you only need to do the following:

### hx711_init

1. Pass the `pio_program_t*` pointer created by `pioasm` in the Pico SDK (automatically done for you when calling `pico_generate_pio_header()` in your `CMakeLists.txt` file).

2. Pass a function pointer to an initialisation function which sets up the PIO program (but does _not_ start it) which takes a pointer to the `hx711_t` struct as its only argument. ie. `void (*hx711_program_init_t)(hx711_t* const)`.

The call to `hx711_init` should look like this:

```c
hx711_init(
    &hx,
    clkPin,
    datPin,
    pio0,
    your_pio_program_created_by_pioasm,
    your_pio_initialisation_function);
```

### Passing HX711 Values From PIO to Code

The two functions for obtaining values, `hx711_get_value` and `hx711_get_value_timeout`, both expect the raw, unsigned value from the HX711 according to the datasheet. There should be 24 bits (ie. 3 bytes) with the most significant bit first.

`hx711_get_value` is a blocking function. It will wait until the RX FIFO is not empty.

`hx711_get_value_timeout` is also blocking function with a timeout. It will watch the RX FIFO until there are at least 3 bytes.

Both functions will subsequently read from and clear the RX FIFO.

### Setting HX711 Gain

`hx711_set_gain` will transmit an unsigned 32 bit integer to the PIO program which represents the gain to set. This integer will be in the range 0 to 2 inclusive, corresponding to a HX711 gain of 128, 32, and 64 respectively.

The function will then perform two sequential PIO reads. The first is a non-blocking read to clear whatever is in the RX FIFO, followed by a blocking read to give the PIO program as long as it needs to finish reading the previously set gain.

### Setting HX711 Power

The PIO program should _not_ attempt to change the HX711's power state.
