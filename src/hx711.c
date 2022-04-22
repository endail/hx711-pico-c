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
#include <stdbool.h>
#include <stdint.h>
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "../include/hx711.h"

void hx711_init(
    hx711_t* const hx,
    const uint clk,
    const uint dat,
    PIO pio,
    const pio_program_t* prog,
    hx711_program_init_t prog_init_func) {

        assert(hx != NULL);
        assert(pio != NULL);
        assert(prog != NULL);
        assert(prog_init_func != NULL);

        mutex_init(&hx->_mut);
        mutex_enter_blocking(&hx->_mut);

        hx->clock_pin = clk;
        hx->data_pin = dat;
        hx->_pio = pio;
        hx->_prog = prog;

        gpio_init(hx->clock_pin);
        gpio_set_dir(hx->clock_pin, GPIO_OUT);
        gpio_put(hx->clock_pin, 0);
        
        gpio_init(hx->data_pin);
        gpio_set_dir(hx->data_pin, GPIO_IN);
        gpio_pull_up(hx->data_pin);

        hx->_offset = pio_add_program(hx->_pio, hx->_prog);
        hx->_state_mach = pio_claim_unused_sm(hx->_pio, true);

        prog_init_func(hx);

        mutex_exit(&hx->_mut);

}

void hx711_close(hx711_t* const hx) {

    assert(hx != NULL);
    assert(mutex_is_initialized(&hx->_mut));

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
    assert(mutex_is_initialized(&hx->_mut));

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

    mutex_exit(&hx->_mut);

}

int32_t hx711_get_value(hx711_t* const hx) {

    assert(hx != NULL);
    assert(mutex_is_initialized(&hx->_mut));

    mutex_enter_blocking(&hx->_mut);

    //block until a value is available
    const uint32_t rawVal = pio_sm_get_blocking(
        hx->_pio,
        hx->_state_mach);

    mutex_exit(&hx->_mut);

    return hx711_get_twos_comp(rawVal);

}

bool hx711_get_value_timeout(
    hx711_t* const hx,
    const absolute_time_t* const timeout,
    int32_t* const val) {
    
        assert(hx != NULL);
        assert(val != NULL);
        assert(mutex_is_initialized(&hx->_mut));
        assert(!is_nil_time(*timeout));

        bool success = false;
        static const unsigned char byteThreshold = HX711_READ_BITS / 8;

        mutex_enter_blocking(&hx->_mut);

        while(!time_reached(*timeout)) {
            if(pio_sm_get_rx_fifo_level(hx->_pio, hx->_state_mach) >= byteThreshold) {
                *val = hx711_get_twos_comp(pio_sm_get(hx->_pio, hx->_state_mach));
                success = true;
                break;
            }
        }

        mutex_exit(&hx->_mut);

        return success;

}

void hx711_set_power(hx711_t* const hx, const hx711_power_t pwr) {

    assert(hx != NULL);
    assert(mutex_is_initialized(&hx->_mut));

    mutex_enter_blocking(&hx->_mut);

    if(pwr == hx711_pwr_up) {
        gpio_put(hx->clock_pin, 0);
        pio_sm_restart(hx->_pio, hx->_state_mach);
        pio_sm_set_enabled(hx->_pio, hx->_state_mach, true);
    }
    else if(pwr == hx711_pwr_down) {
        pio_sm_set_enabled(hx->_pio, hx->_state_mach, false);
        gpio_put(hx->clock_pin, 1);
    }

    mutex_exit(&hx->_mut);

}
