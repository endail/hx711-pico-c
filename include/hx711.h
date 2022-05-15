// MIT License
// 
// Copyright (c) 2022 Daniel Robertson
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _HX711_H_0ED0E077_8980_484C_BB94_AF52973CDC09
#define _HX711_H_0ED0E077_8980_484C_BB94_AF52973CDC09

#include <stdbool.h>
#include <stdint.h>
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint HX711_READ_BITS = 24;

static const uint HX711_POWER_DOWN_TIMEOUT = 60; //us

static const uint HX711_SETTLING_TIMES[] = { //ms
    400,
    50
};

static const uint HX711_RATE_SPS[] = {
    10,
    80
};

typedef enum {
    hx711_rate_10 = 0,
    hx711_rate_80
} hx711_rate_t;

typedef enum {
    hx711_gain_128 = 25, //clock pulse counts
    hx711_gain_32 = 26,
    hx711_gain_64 = 27
} hx711_gain_t;

typedef enum {
    hx711_pwr_up,
    hx711_pwr_down
} hx711_power_t;

typedef struct {

    uint clock_pin;
    uint data_pin;

    PIO _pio;
    const pio_program_t* _prog;
    pio_sm_config _default_config;
    uint _state_mach;
    uint _offset;

    mutex_t _mut;

} hx711_t;

// prototype for init function in .pio file
typedef void (*hx711_program_init_t)(hx711_t* const);

void hx711_init(
    hx711_t* const hx,
    const uint clk,
    const uint dat,
    PIO const pio,
    const pio_program_t* const prog,
    hx711_program_init_t prog_init_func);

void hx711_close(hx711_t* const hx);

void hx711_set_gain(
    hx711_t* const hx,
    const hx711_gain_t gain);

static inline int32_t hx711_get_twos_comp(const uint32_t val) {
    return (int32_t)(-(val & 0x800000)) + (int32_t)(val & 0x7fffff);
}

static inline bool hx711_is_min_saturated(const int32_t val) {
    return val == -0x800000; //−8,388,608
}

static inline bool hx711_is_max_saturated(const int32_t val) {
    return val == 0x7fffff; //8,388,607
}

static inline uint hx711_get_settling_time(const hx711_rate_t rate) {
    return HX711_SETTLING_TIMES[(uint)rate];
}

static inline uint hx711_get_rate_sps(const hx711_rate_t rate) {
    return HX711_RATE_SPS[(uint)rate];
}

int32_t hx711_get_value(hx711_t* const hx);

bool hx711_get_value_timeout(
    hx711_t* const hx,
    const absolute_time_t* const timeout,
    int32_t* const val);

void hx711_set_power(
    hx711_t* const hx,
    const hx711_power_t pwr);

static inline void hx711_wait_settle(const hx711_rate_t rate) {
    sleep_ms(hx711_get_settling_time(rate));
}

static inline void hx711_wait_power_down() {
    sleep_us(HX711_POWER_DOWN_TIMEOUT);
}

#ifdef __cplusplus
}
#endif

#endif
