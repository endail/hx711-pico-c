// MIT License
// 
// Copyright (c) 2023 Daniel Robertson
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
#include <stdlib.h>
#include <string.h>

void hx711_multi_init(
    hx711_multi_t* const hxm,
    const uint clk,
    const uint datPinBase,
    const size_t chips,
    PIO const pio,
    hx711_multi_pio_init_t pioInitFunc,
    const pio_program_t* const awaiterProg,
    hx711_multi_program_init_t awaiterProgInitFunc,
    const pio_program_t* const readerProg,
    hx711_multi_program_init_t readerProgInitFunc) {

        assert(hxm != NULL);
        assert(chips <= HX711_MULTI_MAX_CHIPS);
        assert(pio != NULL);
        assert(pioInitFunc != NULL);
        assert(awaiterProg != NULL);
        assert(awaiterProgInitFunc != NULL);
        assert(readerProg != NULL);
        assert(readerProgInitFunc != NULL);

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
        hxm->_read_buffer = (uint32_t*)malloc(sizeof(uint32_t) * hxm->_chips_len);

        gpio_init(hxm->clock_pin);
        gpio_set_dir(hxm->clock_pin, GPIO_OUT);

        //contiguous
        for(uint i = hxm->data_pin_base; i < hxm->_chips_len; ++i) {
            gpio_init(i);
            gpio_set_dir(i, GPIO_IN);
        }

        pioInitFunc(hxm);
        awaiterProgInitFunc(hxm);
        readerProgInitFunc(hxm);

        //set up dma
        hxm->_dma_reader_channel = dma_claim_unused_channel(true);

        channel_config_set_transfer_data_size(
            &hxm->_dma_reader_config,
            DMA_SIZE_32);

        //reading one word from SM RX FIFO
        channel_config_set_read_increment(
            &hxm->_dma_reader_config,
            false);

        //reading multiple words into array buffer
        channel_config_set_write_increment(
            &hxm->_dma_reader_config,
            true);

        //receive data from reader SM
        channel_config_set_dreq(
            &hxm->_dma_reader_config,
            pio_get_dreq(hxm->_pio, hxm->_reader_sm, false));

        dma_channel_configure(
            hxm->_dma_reader_channel,
            &hxm->_dma_reader_config,
            hxm->_read_buffer,
            &hxm->_pio->rxf[hxm->_reader_sm],
            HX711_READ_BITS, //24 transfers every time
            false); //don't start until requested

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

    dma_channel_abort(hxm->_dma_reader_channel);
    dma_channel_unclaim(hxm->_dma_reader_channel);

    free(hxm->_read_buffer);

    mutex_exit(&hxm->_mut);

}

void hx711_multi_set_gain(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        CHECK_HX711_MULTI_INITD(hxm);

        const uint32_t gainVal = hx711__gain_to_sm_gain(gain);

        mutex_enter_blocking(&hxm->_mut);

        pio_sm_drain_tx_fifo(
            hxm->_pio,
            hxm->_reader_sm);

        pio_sm_put(
            hxm->_pio,
            hxm->_reader_sm,
            gainVal);

        hx711_multi__get_values_raw(hxm);

        mutex_exit(&hxm->_mut);

}

bool hx711_multi_get_values_timeout(
    hx711_multi_t* const hxm,
    const uint timeout,
    int32_t* values) {

        CHECK_HX711_MULTI_INITD(hxm);
        assert(values != NULL);

        bool success;

        mutex_enter_blocking(&hxm->_mut);

        if((success = hx711_multi__wait_app_ready_timeout(hxm->_pio, timeout))) {
            hx711_multi__get_values_raw(hxm);
        }

        mutex_exit(&hxm->_mut);

        if(success) {
            hx711_multi__convert_raw_vals(
                hxm->_read_buffer,
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
        assert(values != NULL);

        mutex_enter_blocking(&hxm->_mut);

        hx711_multi__get_values_raw(hxm);

        hx711_multi__pinvals_to_rawvals(
            hxm->_read_buffer,
            hxm->_read_buffer,
            hxm->_chips_len);

        hx711_multi__convert_raw_vals(
            hxm->_read_buffer,
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

    dma_channel_abort(hxm->_dma_reader_channel);

    gpio_put(
        hxm->clock_pin,
        true);

    mutex_exit(&hxm->_mut);

}

bool hx711_multi__wait_app_ready_timeout(
    PIO const pio,
    const uint timeout) {

        assert(pio != NULL);

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

void hx711_multi__pinvals_to_rawvals(
    uint32_t* pinvals,
    uint32_t* rawvals,
    const size_t len) {

        assert(pinvals != NULL);
        assert(rawvals != NULL);
        assert(len > 0);

        //construct an individual chip value by OR-ing
        //together the bits from the pinvals array.
        //
        //each n-th bit of the pinvals array makes up all
        //the bits for an individual chip. ie.:
        //
        //pinvals[0] contains all the 24th bit HX711 values
        //for each chip, pinvals[1] contains all the 23rd bit
        //values, and so on...
        //
        //(pinvals[0] >> 0) & 1 is the 24th HX711 bit of the 0th chip
        //(pinvals[1] >> 0) & 1 is the 23rd HX711 bit of the 0th chip
        //(pinvals[1] >> 2) & 1 is the 23rd HX711 bit of the 3rd chip
        //(pinvals[23] >> 0) & 1 is the 0th HX711 bit of the 0th chip
        //
        //eg.
        //rawvals[0] = 
        //    ((pinvals[0] >> 0) & 1) << 24 |
        //    ((pinvals[1] >> 0) & 1) << 23 |
        //    ((pinvals[2] >> 0) & 1) << 22 |
        //...
        //    ((pinvals[23]) >> 0) & 1) << 0;

        for(size_t chipNum = 0; chipNum < len; ++chipNum) {
            rawvals[chipNum] = 0; //0-init
            for(size_t bitPos = 0; bitPos < HX711_READ_BITS; ++bitPos) {
                rawvals[chipNum] |= 
                    ((pinvals[bitPos] >> chipNum) & 1)
                    << (HX711_READ_BITS - bitPos - 1);
            }
        }

}
