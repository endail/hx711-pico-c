/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// https://paulbupejr.com/raspberry-pi-pico-windows-development/
/**
 * pg. 28 multicore; mutex
 * 
 * 
 * 
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hx711.pio.h"

// Assumes little endian
void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

enum hx711_rate_t {
    hz_10,
    hz_80,
    other
};

enum hx711_channel_t {
    a,
    b
};

enum hx711_gain_t {
    gain_128 = 25,
    gain_32 = 26,
    gain_64 = 27
};

enum hx711_power_t {
    up = 1,
    down = 0
};

struct hx711_t {
    uint clock_pin;
    uint data_pin;
    hx711_gain_t gain;
    PIO _pio;
    uint _state_mach;
    uint _offset;
};

int hx711_init(hx711_t* const hx, const uint clk, const uint dat) {

    hx->clock_pin = clk;
    hx->data_pin = dat;
    hx->gain = hx711_gain_t::gain_128;

    gpio_init(clk);
    gpio_set_dir(clk, GPIO_OUT);
    gpio_put(clk, 0);
    
    gpio_init(dat);
    gpio_set_dir(dat, GPIO_IN);
    gpio_pull_up(dat);

    hx->_pio = pio0;
    hx->_offset = pio_add_program(hx->_pio, &hx711_program);
    hx->_state_mach = pio_claim_unused_sm(hx->_pio, true);

    hx711_program_init(
        hx->_pio,
        hx->_state_mach,
        hx->_offset,
        clk,
        dat);

    return 0;

}

int hx711_set_config(hx711_t* const hx, const hx711_gain_t gain) {

    pio_sm_put_blocking(
        hx->_pio,
        hx->_state_mach,
        (uint32_t)gain);

    pio_sm_get_blocking(
        hx->_pio,
        hx->_state_mach);

    hx->gain = gain;

    return 0;

}

int32_t hx711_get_value(hx711_t* const hx) {

    pio_sm_put_blocking(
        hx->_pio,
        hx->_state_mach,
        hx->gain);

    const int32_t val = (int32_t)pio_sm_get_blocking(
        hx->_pio,
        hx->_state_mach);

    return -(val & 0x800000) + (val & 0x7fffff);

}

int hx711_set_power(hx711_t* const hx, const hx711_power_t pwr) {
    gpio_put(hx->clock_pin, (bool)pwr);
    return 0;
}

int main() {

    stdio_init_all();

    hx711_t hx;
    hx711_init(&hx, 14, 15);
    
    while(true) {
        printf("%i\n", hx711_get_value(&hx));
    }

    return 0;

}
