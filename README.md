# hx711-pico-c

## General Description

This is my implementation of reading from a HX711 via a Raspberry Pi Pico. It uses the RP2040's PIO feature to be as efficient as possible.

After building, copy `main.uf2` in the build directory to the Raspberry Pi Pico and then open up a serial connection to the Pico at a baud rate of 115200.

I have used [this helpful tutorial](https://paulbupejr.com/raspberry-pi-pico-windows-development/) to setup my Windows environment in order to program the Pico via Visual Studio Code.

![resources/hx711_serialout.gif](resources/hx711_serialout.gif)

The .gif above illustrates the [current example code](main.c) obtaining data from a HX711 operating at 80 samples per second. Each line shows the weight calculated from the median of 3 samples. I applied pressure to the load cell to show the change in weight.

You do not need to use or `#include` the scale functionality if you only want to use the HX711 functions.

## (Brief) Documentation

Not everything is documented here, but this simplified documentation is likely to be the most relevant.

### HX711

```c
enum hx711_rate_t {
    hx711_rate_10,
    hx711_rate_80
};

enum hx711_gain_t {
    hx711_gain_128,
    hx711_gain_32,
    hx711_gain_64
};

enum hx711_power_t {
    hx711_pwr_up,
    hx711_pwr_down
};

struct hx711_t {
    uint clock_pin;
    uint data_pin;
    hx711_gain_t;
};
```

- `void hx711_init( hx711_t* const hx, const uint clk, const uint dat, PIO pio, const pio_program_t* prog, hx711_program_init_t prog_init_func )`

- `void hx711_close( hx711_t* const hx )`

- `void hx711_set_gain( hx711_t* const hx, const hx711_gain_t gain )`

- `int32_t hx711_get_value( hx711_t* const hx )`

- `void hx711_set_power( hx711_t* const hx, const hx711_power_t pwr )`

### Mass

```c
enum mass_unit_t {
    mass_ug,
    mass_mg,
    mass_g,
    mass_kg,
    mass_ton,
    mass_imp_ton,
    mass_us_ton,
    mass_st,
    mass_lb,
    mass_oz
};

struct mass_t {
    double ug;
    mass_unit_t unit;
};

```

- `const char* const mass_unit_to_string( const mass_unit_t u )`

- `const double* const mass_unit_to_ratio( const mass_unit_t u )`

- `void mass_get_value( const mass_t* const m, double* const val )`

- `void mass_set_value( mass_t* const m, const mass_unit_t unit, const double* const val )`

- `void mass_to_string( const mass_t* const m, char* const buff )`

### Scale

```c
enum strategy_type_t {
    strategy_type_samples,
    strategy_type_time
};

enum read_type_t {
    read_type_median,
    read_type_average
};

struct scale_options_t {
    strategy_type_t strat;
    read_type_t read;
    size_t samples;
    uint64_t timeout;
};

struct scale_t {
    int32_t offset;
    int32_t ref_unit;
    mass_unit_t unit;
};
```

- `void scale_init( scale_t* const sc, hx711_t* const hx, const int32_t offset, const int32_t ref_unit, const mass_unit_t unit )`

- `void scale_zero( scale_t* const sc, const scale_options_t* const opt )`

- `void scale_weight( scale_t* const sc, mass_t* const m, const scale_options_t* const opt )`

