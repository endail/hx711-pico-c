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

#include "../include/hx711_multi.h"
#include "hardware/gpio.h"
#include <string.h>

void hx711_multi_init(
    hx711_multi_t* const hxm,
    const uint clk,
    const uint datPinBase,
    const uint chips,
    PIO const pio,
    hx711_multi_pio_init_t pioInitFunc,
    const pio_program_t* const awaiterProg,
    hx711_multi_program_init_t awaiterProgInitFunc,
    const pio_program_t* const readerProg,
    hx711_multi_program_init_t readerProgInitFunc) {

        mutex_init(&hxm->_mut);
        mutex_enter_blocking(&hxm->_mut);

        hxm->clock_pin = clk;
        hxm->data_pin_base = datPinBase;
        hxm->_chips_len = chips;

        hxm->_pio = pio;
        hxm->_awaiter_prog = awaiterProg;
        hxm->_reader_prog = readerProg;

        hxm->_awaiter_offset = pio_add_program(hxm->_pio, hxm->_awaiter_prog);
        hxm->_awaiter_sm = (uint)pio_claim_unused_sm(hxm->_pio, true);

        hxm->_reader_offset = pio_add_program(hxm->_pio, hxm->_reader_prog);
        hxm->_reader_sm = (uint)pio_claim_unused_sm(hxm->_pio, true);

        gpio_init(hxm->clock_pin);
        gpio_set_dir(hxm->clock_pin, GPIO_OUT);

        for(uint i = hxm->data_pin_base; i < hxm->_chips_len; ++i) {
            gpio_init(i);
            gpio_set_dir(i, GPIO_IN);
        }

        pioInitFunc(hxm);
        awaiterProgInitFunc(hxm);
        readerProgInitFunc(hxm);

        mutex_exit(&hxm->_mut);

}

void hx711_multi_close(hx711_multi_t* const hxm) {

    CHECK_HX711_MULTI_INITD(hxm);

    mutex_enter_blocking(&hxm->_mut);

    pio_sm_set_enabled(hxm->_pio, hxm->_awaiter_sm, false);
    pio_sm_set_enabled(hxm->_pio, hxm->_reader_sm, false);

    pio_sm_unclaim(hxm->_pio, hxm->_awaiter_sm);
    pio_sm_unclaim(hxm->_pio, hxm->_reader_sm);

    pio_remove_program(
        hxm->_pio,
        hxm->_awaiter_prog,
        hxm->_awaiter_offset);

    pio_remove_program(hxm->_pio,
        hxm->_reader_prog,
        hxm->_reader_offset);

    mutex_exit(&hxm->_mut);

}

void hx711_multi_set_gain(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        CHECK_HX711_MULTI_INITD(hxm);

        const uint32_t gainVal = hx711__gain_to_sm_gain(gain);
        uint32_t dummy[HX711_MULTI_MAX_CHIPS];

        mutex_enter_blocking(&hxm->_mut);

        pio_sm_drain_tx_fifo(
            hxm->_pio,
            hxm->_reader_sm);

        pio_sm_put(
            hxm->_pio,
            hxm->_reader_sm,
            gainVal);

        hx711_multi__get_values_raw(
            hxm->_pio,
            hxm->_reader_sm,
            dummy);

        mutex_exit(&hxm->_mut);

}

bool hx711_multi_get_values_timeout(
    hx711_multi_t* const hxm,
    const uint timeout,
    int32_t* values) {

        CHECK_HX711_MULTI_INITD(hxm);

        bool success;
        uint32_t rawVals[HX711_MULTI_MAX_CHIPS];

        memset(
            rawVals,
            0,
            sizeof(rawVals[0]) * HX711_MULTI_MAX_CHIPS);

        mutex_enter_blocking(&hxm->_mut);

        if((success = hx711_multi__wait_app_ready_timeout(hxm->_pio, timeout))) {
            hx711_multi__read_into_array(hxm->_pio, hxm->_reader_sm, rawVals);
        }

        mutex_exit(&hxm->_mut);

        if(success) {
            hx711_multi__convert_raw_vals(
                rawVals,
                values,
                hxm->_chips_len);
        }

        return success;

}

/**
 * @brief Fill an array with one value from each chip
 * 
 * @param hxm 
 * @param values 
 */
void hx711_multi_get_values(
    hx711_multi_t* const hxm,
    int32_t* values) {

        CHECK_HX711_MULTI_INITD(hxm);

        uint32_t rawVals[HX711_MULTI_MAX_CHIPS];

        memset(
            rawVals,
            0,
            sizeof(rawVals[0]) * HX711_MULTI_MAX_CHIPS);

        mutex_enter_blocking(&hxm->_mut);

        hx711_multi__get_values_raw(
            hxm->_pio,
            hxm->_reader_sm,
            rawVals);

        hx711_multi__convert_raw_vals(
            rawVals,
            values,
            hxm->_chips_len);

        mutex_exit(&hxm->_mut);

}

void hx711_multi_power_up(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        CHECK_HX711_MULTI_INITD(hxm);

        const uint32_t gainVal = hx711__gain_to_sm_gain(gain);

        mutex_enter_blocking(&hxm->_mut);

        gpio_put(
            hxm->clock_pin,
            false);

        pio_sm_init(
            hxm->_pio,
            hxm->_reader_sm,
            hxm->_reader_offset,
            &hxm->_reader_default_config);

        pio_sm_put(
            hxm->_pio,
            hxm->_reader_sm,
            gainVal);

        pio_sm_init(
            hxm->_pio,
            hxm->_awaiter_sm,
            hxm->_awaiter_offset,
            &hxm->_awaiter_default_config);

        //start the awaiter
        pio_sm_set_enabled(
            hxm->_pio,
            hxm->_awaiter_sm,
            true);

        //start the reader
        pio_sm_set_enabled(
            hxm->_pio,
            hxm->_reader_sm,
            true);

        mutex_exit(&hxm->_mut);

}

void hx711_multi_power_down(hx711_multi_t* const hxm) {

    CHECK_HX711_MULTI_INITD(hxm);

    mutex_enter_blocking(&hxm->_mut);

    pio_sm_set_enabled(
        hxm->_pio,
        hxm->_awaiter_sm,
        false);

    pio_sm_set_enabled(
        hxm->_pio,
        hxm->_reader_sm,
        false);

    gpio_put(
        hxm->clock_pin,
        true);

    mutex_exit(&hxm->_mut);

}

bool hx711_multi__wait_app_ready_timeout(
    PIO const pio,
    const uint timeout) {

        bool success = false;
        const absolute_time_t endTime = make_timeout_time_us(timeout);

        assert(!is_nil_time(endTime));

        while(!time_reached(endTime)) {
            if(pio_interrupt_get(pio, _HX711_MULTI_APP_WAIT_IRQ_NUM)) {
                success = true;
                break;
            }
        }

        if(success) {
            pio_interrupt_clear(pio, _HX711_MULTI_APP_WAIT_IRQ_NUM);
        }

        return success;

}

void hx711_multi__read_into_array(
    PIO const pio,
    const uint sm,
    uint32_t* values) {

        uint32_t pinBits;
        bool bit;

        //read 24 times
        for(uint i = 0; i < HX711_READ_BITS; ++i) {

            //read 13 bits of pin values
            //each bit is one pin's value
            pinBits = pio_sm_get_blocking(pio, sm);
            
            //iterate over the 13 bits
            for(uint j = 0; i < HX711_MULTI_MAX_CHIPS; ++j) {
                //iterate over all chips
                //set the i-th bit of the j-th chip
                bit = (pinBits >> j) & 1;
                values[j] = values[j] & (~(1 << i) | (bit << j));
            }
        }

}
