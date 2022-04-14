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

#ifndef _HX711_H_
#define _HX711_H_

#include "hardware/pio.h"
#include "pico/sync.h"

const uint8_t HX711_READ_BITS;

typedef enum {
    hz_10,
    hz_80,
    other
} hx711_rate_t;

typedef enum {
    a,
    b
} hx711_channel_t;

typedef enum {
    gain_128 = 25,
    gain_32 = 26,
    gain_64 = 27
} hx711_gain_t;

typedef enum {
    up = 1,
    down = 0
} hx711_power_t;

typedef struct {

    uint clock_pin;
    uint data_pin;
    hx711_gain_t gain;
    
    PIO _pio;
    const pio_program_t* _prog;
    uint _state_mach;
    uint _offset;

    semaphore_t _sem;

} hx711_t;

typedef void (*program_init_t)(hx711_t* const);

void hx711_init(
    hx711_t* const hx,
    const uint clk,
    const uint dat,
    PIO pio,
    const pio_program_t* prog,
    program_init_t);

void hx711_close(hx711_t* const hx);

void hx711_set_config(
    hx711_t* const hx,
    const hx711_gain_t gain);

int32_t hx711_get_twos_comp(const uint32_t val);

bool hx711_is_min_saturated(const int32_t val);

bool hx711_is_max_saturated(const int32_t val);

int32_t hx711_get_value(hx711_t* const hx);

int32_t hx711_get_value_fast(hx711_t* const hx);

void hx711_set_power(
    hx711_t* const hx,
    const hx711_power_t pwr);

#endif
