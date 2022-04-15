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

#include <assert.h>
#include "../include/hx711.h"

const uint8_t HX711_READ_BITS = 24;

void hx711_init(
    hx711_t* const hx,
    const uint clk,
    const uint dat,
    PIO pio,
    const pio_program_t* prog,
    hx711_program_init_t prog_init) {

        sem_init(
            &hx->_sem,
            0,
            1);

        hx->clock_pin = clk;
        hx->data_pin = dat;
        hx->gain = gain_128;

        gpio_init(clk);
        gpio_set_dir(clk, GPIO_OUT);
        gpio_put(clk, 0);
        
        gpio_init(dat);
        gpio_set_dir(dat, GPIO_IN);
        gpio_pull_up(dat);

        hx->_pio = pio;
        hx->_prog = prog;
        hx->_offset = pio_add_program(hx->_pio, hx->_prog);
        hx->_state_mach = pio_claim_unused_sm(hx->_pio, true);

        prog_init(hx);

        sem_release(&hx->_sem);

}

void hx711_close(hx711_t* const hx) {

    sem_acquire_blocking(&hx->_sem);

    pio_sm_set_enabled(
        hx->_pio,
        hx->_state_mach,
        false);

    pio_sm_unclaim(
        hx->_pio,
        hx->_state_mach);

    pio_remove_program(
        hx->_pio,
        hx->_prog,
        hx->_offset);

    gpio_disable_pulls(hx->data_pin);

    gpio_set_input_enabled(
        hx->data_pin,
        false);

    sem_release(&hx->_sem);

}

void hx711_set_gain(hx711_t* const hx, const hx711_gain_t gain) {

    sem_acquire_blocking(&hx->_sem);

    pio_sm_put_blocking(
        hx->_pio,
        hx->_state_mach,
        (((uint32_t)gain) - HX711_READ_BITS) - 1);

    pio_sm_get_blocking(
        hx->_pio,
        hx->_state_mach);

    hx->gain = gain;

    sem_release(&hx->_sem);

}

int32_t hx711_get_twos_comp(const uint32_t val) {
    return (int32_t)(-(val & 0x800000) + (val & 0x7fffff));
}

bool hx711_is_min_saturated(const int32_t val) {
    return val == 0x7fffff;
}

bool hx711_is_max_saturated(const int32_t val) {
    return val == 0x800000;
}

int32_t hx711_get_value(hx711_t* const hx) {

    sem_acquire_blocking(&hx->_sem);

    //block until a value is available
    const uint32_t val = pio_sm_get_blocking(
        hx->_pio,
        hx->_state_mach);

    sem_release(&hx->_sem);

    return hx711_get_twos_comp(val & 0xffffff);

}

void hx711_set_power(hx711_t* const hx, const hx711_power_t pwr) {
    sem_acquire_blocking(&hx->_sem);
    gpio_put(hx->clock_pin, (bool)pwr);
    sem_release(&hx->_sem);
}
