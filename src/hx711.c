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
#include <stdint.h>
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "../include/hx711.h"

const uint8_t HX711_READ_BITS = 24;

const uint8_t HX711_SLEEP_TIMEOUT = 60;

const uint16_t HX711_SETTLING_TIMES[] = {
    400,
    50
};

void hx711_init(
    hx711_t* const hx,
    const uint clk,
    const uint dat,
    PIO pio,
    const pio_program_t* prog,
    hx711_program_init_t prog_init_func) {

        assert(hx != NULL);
        assert(prog != NULL);

        mutex_init(&hx->_mut);
        mutex_enter_blocking(&hx->_mut);

        hx->clock_pin = clk;
        hx->data_pin = dat;
        hx->gain = hx711_gain_128;

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

        prog_init_func(hx);

        mutex_exit(&hx->_mut);

}

void hx711_close(hx711_t* const hx) {

    assert(hx != NULL);

    mutex_enter_blocking(&hx->_mut);

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

    mutex_exit(&hx->_mut);

}

void hx711_set_gain(hx711_t* const hx, const hx711_gain_t gain) {

    assert(hx != NULL);

    mutex_enter_blocking(&hx->_mut);

    /**
     * gain value is 0-based and calculated by:
     * gain = clock pulses - 24 - 1
     * ie. gain of 128 is 25 clock pulses, so
     * gain = 25 - 24 - 1
     * gain = 0
     */
    pio_sm_put_blocking(
        hx->_pio,
        hx->_state_mach,
        (((uint32_t)gain) - HX711_READ_BITS) - 1);

    //should not need to obtain a value, as the gain should
    //now be set for the next value

    hx->gain = gain;

    mutex_exit(&hx->_mut);

}

int32_t hx711_get_twos_comp(int32_t val) {
    val = val & 0xffffff; // only use bottom 24 bits
    return -(val & 0x800000) + (val & 0x7fffff);
}

bool hx711_is_min_saturated(const int32_t val) {
    return val == -0x800000; //−8,388,608
}

bool hx711_is_max_saturated(const int32_t val) {
    return val == 0x7fffff; //8,388,607
}

uint16_t hx711_get_settling_time(const hx711_rate_t rate) {
    return HX711_SETTLING_TIMES[(int)rate];
}

int32_t hx711_get_value(hx711_t* const hx) {

    assert(hx != NULL);

    mutex_enter_blocking(&hx->_mut);

    //block until a value is available
    uint32_t rawVal = pio_sm_get_blocking(
        hx->_pio,
        hx->_state_mach);

    mutex_exit(&hx->_mut);

    return hx711_get_twos_comp(rawVal);

}

bool hx711_get_value_timeout(
    hx711_t* const hx,
    const absolute_time_t* const timeout,
    int32_t* val) {
    
        assert(hx != NULL);
        assert(val != NULL);

        bool didTimeout = true;

        mutex_enter_blocking(&hx->_mut);

        while(!time_reached(*timeout)) {
            //check if 24 bits available
            if(pio_sm_get_rx_fifo_level(hx->_pio, hx->_state_mach) >= HX711_READ_BITS) {
                didTimeout = false;
                *val = hx711_get_twos_comp(pio_sm_get(hx->_pio, hx->_state_mach));
                break;
            }
        }

        mutex_exit(&hx->_mut);

        return didTimeout;

}

void hx711_set_power(hx711_t* const hx, const hx711_power_t pwr) {
    
    assert(hx != NULL);
    
    mutex_enter_blocking(&hx->_mut);
    gpio_put(hx->clock_pin, (bool)pwr);
    mutex_exit(&hx->_mut);

}
