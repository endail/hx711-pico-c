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
#include "../include/util.h"
#include "hardware/gpio.h"

void hx711_multi_init(
    hx711_multi_t* const hxm,
    const hx711_multi_config_t* const config) {

        assert(hxm != NULL);
        assert(config != NULL);
        
        assert(config->chips_len >= HX711_MULTI_MIN_CHIPS);
        assert(config->chips_len <= HX711_MULTI_MAX_CHIPS);
        
        assert(config->pio != NULL);
        assert(config->pio_init != NULL);
        
        assert(config->awaiter_prog != NULL);
        assert(config->awaiter_prog_init != NULL);

        assert(config->reader_prog != NULL);
        assert(config->reader_prog_init != NULL);

        mutex_init(&hxm->_mut);
        mutex_enter_blocking(&hxm->_mut);

        hxm->_clock_pin = config->clock_pin;
        hxm->_data_pin_base = config->data_pin_base;
        hxm->_chips_len = config->chips_len;

        hxm->_pio = config->pio;
        hxm->_awaiter_prog = config->awaiter_prog;
        hxm->_reader_prog = config->reader_prog;

        hxm->_awaiter_offset = pio_add_program(
            hxm->_pio,
            hxm->_awaiter_prog);

        hxm->_awaiter_sm = (uint)pio_claim_unused_sm(
            hxm->_pio,
            true);

        hxm->_reader_offset = pio_add_program(
            hxm->_pio,
            hxm->_reader_prog);

        hxm->_reader_sm = (uint)pio_claim_unused_sm(
            hxm->_pio,
            true);

        util_gpio_set_output_enabled(hxm->_clock_pin);

        util_gpio_set_contiguous_input_pins(
            hxm->_data_pin_base,
            hxm->_chips_len);

        config->pio_init(hxm);
        config->awaiter_prog_init(hxm);
        config->reader_prog_init(hxm);

        hxm->_dma_channel = dma_claim_unused_channel(true);

        dma_channel_config cfg = dma_channel_get_default_config(
            hxm->_dma_channel);

        channel_config_set_transfer_data_size(
            &cfg,
            DMA_SIZE_32);   //pio fifo output is 32 bits

        channel_config_set_read_increment(
            &cfg,
            false);

        channel_config_set_write_increment(
            &cfg,
            true);

        //do NOT set ring buffer
        //ie. do not use channel_config_set_ring
        //if, for whatever reason, the DMA transfer
        //fails, subsequent transfer invocations
        //will reset the write address

        channel_config_set_dreq(
            &cfg,
            pio_get_dreq(
                hxm->_pio,
                hxm->_reader_sm, false));

        dma_channel_configure(
            hxm->_dma_channel,
            &cfg,
            hxm->_read_buffer,                  //write to buffer
            &hxm->_pio->rxf[hxm->_reader_sm],   //read from reader pio program rx fifo
            HX711_READ_BITS,                    //24 transfers; one for each HX711 bit
            false);                             //false = don't start now

        mutex_exit(&hxm->_mut);

}

void hx711_multi_close(hx711_multi_t* const hxm) {

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

    pio_sm_unclaim(
        hxm->_pio,
        hxm->_awaiter_sm);

    pio_sm_unclaim(
        hxm->_pio,
        hxm->_reader_sm);

    pio_remove_program(
        hxm->_pio,
        hxm->_awaiter_prog,
        hxm->_awaiter_offset);

    pio_remove_program(
        hxm->_pio,
        hxm->_reader_prog,
        hxm->_reader_offset);

    dma_channel_abort(
        hxm->_dma_channel);

    dma_channel_unclaim(
        hxm->_dma_channel);

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

void hx711_multi_get_values(
    hx711_multi_t* const hxm,
    int32_t* values) {

        CHECK_HX711_MULTI_INITD(hxm);
        assert(values != NULL);

        mutex_enter_blocking(&hxm->_mut);

        hx711_multi__get_values_raw(hxm);

        //probable race condition with read_buffer if
        //this is moved outside of the mutex
        hx711_multi__pinvals_to_values(
            hxm->_read_buffer,
            values,
            hxm->_chips_len);

        mutex_exit(&hxm->_mut);

}

bool hx711_multi_get_values_timeout(
    hx711_multi_t* const hxm,
    int32_t* values,
    const uint timeout) {

        CHECK_HX711_MULTI_INITD(hxm);
        assert(values != NULL);

        mutex_enter_blocking(&hxm->_mut);

        if(!hx711_multi__get_values_timeout_raw(hxm, timeout)) {
            return false;
        }

        //probable race condition with read_buffer if
        //this is moved outside of the mutex
        hx711_multi__pinvals_to_values(
            hxm->_read_buffer,
            values,
            hxm->_chips_len);

        mutex_exit(&hxm->_mut);

        return true;

}

void hx711_multi_power_up(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        CHECK_HX711_MULTI_INITD(hxm);

        const uint32_t gainVal = hx711__gain_to_sm_gain(gain);

        mutex_enter_blocking(&hxm->_mut);

        gpio_put(
            hxm->_clock_pin,
            false);

        pio_sm_init(
            hxm->_pio,
            hxm->_reader_sm,
            hxm->_reader_offset,
            &hxm->_reader_default_config);

        //put the gain value into the reader FIFO
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

    //stop checking for data readiness
    pio_sm_set_enabled(
        hxm->_pio,
        hxm->_awaiter_sm,
        false);

    //stop reading values
    pio_sm_set_enabled(
        hxm->_pio,
        hxm->_reader_sm,
        false);

    gpio_put(
        hxm->_clock_pin,
        true);

    mutex_exit(&hxm->_mut);

}

void hx711_multi__wait_app_ready(hx711_multi_t* const hxm) {

    CHECK_HX711_MULTI_INITD(hxm)

    util_pio_interrupt_wait_clear(
        hxm->_pio,
        HX711_MULTI_APP_WAIT_IRQ_NUM);

}

bool hx711_multi__wait_app_ready_timeout(
    hx711_multi_t* const hxm,
    const uint timeout) {

        CHECK_HX711_MULTI_INITD(hxm)

        return util_pio_interrupt_wait_clear_timeout(
            hxm->_pio,
            HX711_MULTI_APP_WAIT_IRQ_NUM,
            timeout);

/*
        bool success = false;
        const absolute_time_t endTime = make_timeout_time_us(timeout);

        assert(!is_nil_time(endTime));

        //give sm time to return to wait app ready
        while(!time_reached(endTime)) {
            if((success = pio_interrupt_get(hxm->_pio, HX711_MULTI_APP_WAIT_IRQ_NUM))) {
                break;
            }
        }

        //if the above failed in time, return false
        if(!success) {
            return false;
        }

        //start the reading process
        pio_interrupt_clear(
            hxm->_pio,
            HX711_MULTI_APP_WAIT_IRQ_NUM);

        return true;
*/

}

void hx711_multi__pinvals_to_values(
    const uint32_t* const pinvals,
    int32_t* const values,
    const size_t len) {

        assert(pinvals != NULL);
        assert(values != NULL);
        assert((void*)values != (void*)pinvals);
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

            //reset to 0
            //this is the raw value for an individual chip
            uint32_t rawVal = 0;

            //reconstruct an individual twos comp HX711 value from pinbits
            for(size_t bitPos = 0; bitPos < HX711_READ_BITS; ++bitPos) {
                const uint shift = HX711_READ_BITS - bitPos - 1;
                const uint32_t bit = (pinvals[bitPos] >> chipNum) & 1;
                rawVal |= bit << shift;
            }

            //then convert to a regular ones comp
            values[chipNum] = hx711_get_twos_comp(rawVal);

        }

}

void hx711_multi__get_values_raw(
    hx711_multi_t* const hxm) {

        CHECK_HX711_MULTI_INITD(hxm)
        assert(pio_sm_get_rx_fifo_level(hxm->_pio, hxm->_reader_sm) == 0);
        assert(!dma_channel_is_busy(hxm->_dma_channel));

        hx711_multi__wait_app_ready(hxm);

        dma_channel_set_write_addr(
            hxm->_dma_channel,
            hxm->_read_buffer,
            true);

        dma_channel_wait_for_finish_blocking(
            hxm->_dma_channel);

        //there should be nothing left to read
        assert(util_dma_get_transfer_count_remaining(hxm->_dma_channel) == 0);
        assert(pio_sm_get_rx_fifo_level(hxm->_pio, hxm->_reader_sm) == 0);

}

bool hx711_multi__get_values_timeout_raw(
    hx711_multi_t* const hxm,
    const uint timeout) {

        const absolute_time_t endTime = make_timeout_time_us(timeout);

        assert(!is_nil_time(endTime));
        assert(pio_sm_get_rx_fifo_level(hxm->_pio, hxm->_reader_sm) == 0);
        assert(!dma_channel_is_busy(hxm->_dma_channel));

        //don't include app wait in timeout period
        hx711_multi__wait_app_ready(hxm);

        dma_channel_set_write_addr(
            hxm->_dma_channel,
            hxm->_read_buffer,
            true);

        while(!time_reached(endTime)) {
            if(!dma_channel_is_busy(hxm->_dma_channel)) {
                assert(util_dma_get_transfer_count_remaining(hxm->_dma_channel) == 0);
                assert(pio_sm_get_rx_fifo_level(hxm->_pio, hxm->_reader_sm) == 0);
                return true;
            }
        }

        //assume an error
        dma_channel_abort(hxm->_dma_channel);

        return false;

}
