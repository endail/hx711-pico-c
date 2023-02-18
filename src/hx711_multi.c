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

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "pico/platform.h"
#include "pico/time.h"
#include "pico/types.h"
#include "../include/hx711.h"
#include "../include/hx711_multi.h"
#include "../include/util.h"

hx711_multi_async_request_t* hx711_multi__async_request_map[] = {
    NULL
    , //...
};

void hx711_multi_pinvals_to_values(
    const uint32_t* const pinvals,
    int32_t* const values,
    const size_t len) {

        assert(pinvals != NULL);
        assert(values != NULL);
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

            assert(hx711_is_value_valid(values[chipNum]));

        }

}

static bool hx711_multi__is_initd(hx711_multi_t* const hxm) {
    return hxm != NULL &&
        hxm->_pio != NULL &&
        pio_sm_is_claimed(hxm->_pio, hxm->_awaiter_sm) &&
        pio_sm_is_claimed(hxm->_pio, hxm->_reader_sm) &&
        dma_channel_is_claimed(hxm->_dma_channel) &&
        mutex_is_initialized(&hxm->_mut);
}

static bool hx711_multi__is_state_machines_enabled(
    hx711_multi_t* const hxm) {
        return hx711_multi__is_initd(hxm) &&
            util_pio_sm_is_enabled(hxm->_pio, hxm->_awaiter_sm) &&
            util_pio_sm_is_enabled(hxm->_pio, hxm->_reader_sm);
}

void hx711_multi__get_values_raw(
    hx711_multi_t* const hxm,
    uint32_t* const pinvals) {

        assert(hx711_multi__is_initd(hxm));
        assert(hx711_multi__is_state_machines_enabled(hxm));
        assert(pinvals != NULL);

        //DMA should not be active
        assert(!dma_channel_is_busy(hxm->_dma_channel));

        //wait for any current conversion period to end
        util_pio_interrupt_wait(
            hxm->_pio,
            HX711_MULTI_CONVERSION_DONE_IRQ_NUM);

        //clear any residual data
        util_pio_sm_clear_rx_fifo(
            hxm->_pio,
            hxm->_reader_sm);

        //at this stage there is no need to wait for the
        //conversion period irq to be set, just let DMA
        //handle the writes when they begin

        dma_channel_set_write_addr(
            hxm->_dma_channel,
            pinvals,
            true);

        dma_channel_wait_for_finish_blocking(
            hxm->_dma_channel);

        //there should be nothing left to read
        assert(util_dma_get_transfer_count(hxm->_dma_channel) == 0);

}

bool hx711_multi__get_values_timeout_raw(
    hx711_multi_t* const hxm,
    uint32_t* const pinvals,
    const absolute_time_t* const end) {

        assert(hx711_multi__is_initd(hxm));
        assert(hx711_multi__is_state_machines_enabled(hxm));
        assert(pinvals != NULL);
        assert(end != NULL);
        assert(!is_nil_time(*end));
        assert(!dma_channel_is_busy(hxm->_dma_channel));

        //wait for any current conversion period to end to
        //be able to sync with the next period
        if(!util_pio_interrupt_wait_cleared_timeout(
            hxm->_pio,
            HX711_MULTI_CONVERSION_DONE_IRQ_NUM,
            end)) {
                //failed to reach conversion period end within
                //timeout
                return false;
        }

        util_pio_sm_clear_rx_fifo(
            hxm->_pio,
            hxm->_reader_sm);

        dma_channel_set_write_addr(
            hxm->_dma_channel,
            pinvals,
            true);

        const bool isDone = util_dma_channel_wait_for_finish_timeout(
            hxm->_dma_channel,
            end);

        if(!isDone) {
            //assume an error
            dma_channel_abort(hxm->_dma_channel);
        }
#ifndef NDEBUG
        else {
            assert(util_dma_get_transfer_count(hxm->_dma_channel) == 0);
            //enabling this assertion would lead to a race condition with the FIFO
            //assert(pio_sm_is_rx_fifo_empty(hxm->_pio, hxm->_reader_sm));
        }
#endif

        return isDone;

}

void hx711_multi_init(
    hx711_multi_t* const hxm,
    const hx711_multi_config_t* const config) {

        assert(!hx711_multi__is_initd(hxm));

        assert(hxm != NULL);
        assert(config != NULL);

        assert(util_uint_in_range(
            config->chips_len,
            HX711_MULTI_MIN_CHIPS,
            HX711_MULTI_MAX_CHIPS));

        assert(config->pio != NULL);
        check_pio_param(config->pio);
        assert(config->pio_init != NULL);

        assert(config->awaiter_prog != NULL);
        assert(config->awaiter_prog_init != NULL);

        assert(config->reader_prog != NULL);
        assert(config->reader_prog_init != NULL);

#ifndef NDEBUG
        {
            //make sure none of the data pins are also the clock pin
            const uint l = config->data_pin_base + config->chips_len - 1;
            for(uint i = config->data_pin_base; i <= l; ++i) {
                assert(i != config->clock_pin);
            }
        }
#endif

        mutex_init(&hxm->_mut);

        UTIL_MUTEX_BLOCK(hxm->_mut, 

            hxm->_clock_pin = config->clock_pin;
            hxm->_data_pin_base = config->data_pin_base;
            hxm->_chips_len = config->chips_len;

            hxm->_pio = config->pio;
            hxm->_awaiter_prog = config->awaiter_prog;
            hxm->_reader_prog = config->reader_prog;

            //adding programs and claiming state machines
            //will panic if unable
            hxm->_awaiter_offset = pio_add_program(
                hxm->_pio,
                hxm->_awaiter_prog);

            hxm->_reader_offset = pio_add_program(
                hxm->_pio,
                hxm->_reader_prog);

            hxm->_awaiter_sm = (uint)pio_claim_unused_sm(
                hxm->_pio,
                true);

            hxm->_reader_sm = (uint)pio_claim_unused_sm(
                hxm->_pio,
                true);

            util_gpio_set_output(hxm->_clock_pin);

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
                false);         //always read from same location

            channel_config_set_write_increment(
                &cfg,
                true);          //write to next array position

            /**
             * Do not set ring buffer.
             * ie. do not use channel_config_set_ring.
             * If, for whatever reason, the DMA transfer
             * fails, subsequent transfer invocations
             * will reset the write address.
             */

            channel_config_set_dreq(
                &cfg,
                pio_get_dreq(
                    hxm->_pio,
                    hxm->_reader_sm,
                    false));

            dma_channel_configure(
                hxm->_dma_channel,
                &cfg,
                NULL,                               //don't set a write address yet
                &hxm->_pio->rxf[hxm->_reader_sm],   //read from reader pio program rx fifo
                HX711_READ_BITS,                    //24 transfers; one for each HX711 bit
                false);                             //false = don't start now

        );

}

void hx711_multi_close(hx711_multi_t* const hxm) {

    assert(hx711_multi__is_initd(hxm));

    UTIL_MUTEX_BLOCK(hxm->_mut, 

        pio_set_sm_mask_enabled(
            hxm->_pio,
            (1 << hxm->_awaiter_sm) | (1 << hxm->_reader_sm),
            false);

        dma_channel_abort(
            hxm->_dma_channel);

        dma_channel_unclaim(
            hxm->_dma_channel);

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

    );

}

void hx711_multi_set_gain(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        assert(hx711_multi__is_initd(hxm));
        assert(hx711_multi__is_state_machines_enabled(hxm));

        uint32_t dummy[HX711_READ_BITS];
        const uint32_t gainVal = hx711_gain_to_pio_gain(gain);

        assert(hx711_is_pio_gain_valid(gain));

        UTIL_MUTEX_BLOCK(hxm->_mut, 

            pio_sm_drain_tx_fifo(
                hxm->_pio,
                hxm->_reader_sm);

            pio_sm_put(
                hxm->_pio,
                hxm->_reader_sm,
                gainVal);

            hx711_multi__get_values_raw(
                hxm,
                dummy);

        );

}

void hx711_multi_get_values(
    hx711_multi_t* const hxm,
    int32_t* const values) {

        assert(hx711_multi__is_initd(hxm));
        assert(hx711_multi__is_state_machines_enabled(hxm));
        assert(values != NULL);

        uint32_t pinvals[HX711_READ_BITS];

        UTIL_MUTEX_BLOCK(hxm->_mut, 
            hx711_multi__get_values_raw(
                hxm,
                pinvals);
        );

        hx711_multi_pinvals_to_values(
            pinvals,
            values,
            hxm->_chips_len);

}

bool hx711_multi_get_values_timeout(
    hx711_multi_t* const hxm,
    int32_t* const values,
    const uint timeout) {

        assert(hx711_multi__is_initd(hxm));
        assert(hx711_multi__is_state_machines_enabled(hxm));
        assert(values != NULL);

        uint32_t pinvals[HX711_READ_BITS];
        const absolute_time_t end = make_timeout_time_us(timeout);

        bool success;

        UTIL_MUTEX_BLOCK(hxm->_mut, 
            success = hx711_multi__get_values_timeout_raw(
                hxm,
                pinvals,
                &end);
        );

        if(success) {
            hx711_multi_pinvals_to_values(
                pinvals,
                values,
                hxm->_chips_len);
        }

        return success;

}

bool hx711_multi__async_pio_irq_is_set(
    hx711_multi_async_request_t* const req) {

        assert(req != NULL);

        return pio_interrupt_get(
            req->_hxm->_pio,
            HX711_MULTI_CONVERSION_DONE_IRQ_NUM);

}

bool hx711_multi__async_dma_irq_is_set(
    hx711_multi_async_request_t* const req) {

        assert(req != NULL);

        return dma_irqn_get_channel_status(
            req->dma_irq_index,
            req->_hxm->_dma_channel);

}

hx711_multi_async_request_t* const hx711_multi__async_get_dma_irq_request() {

    for(uint i = 0; i < HX711_MULTI_ASYNC_REQ_COUNT; ++i) {
        if(hx711_multi__async_request_map[i] != NULL) {
            if(hx711_multi__async_dma_irq_is_set(hx711_multi__async_request_map[i])) {
                return hx711_multi__async_request_map[i];
            }
        }
    }

    return NULL;

}

hx711_multi_async_request_t* const hx711_multi__async_get_pio_irq_request() {

    for(uint i = 0; i < HX711_MULTI_ASYNC_REQ_COUNT; ++i) {
        if(hx711_multi__async_request_map[i] == NULL) {
            if(hx711_multi__async_pio_irq_is_set(hx711_multi__async_request_map[i])) {
                return hx711_multi__async_request_map[i];
            }
        }
    }

    return NULL;

}

void hx711_multi__async_start_dma(
    hx711_multi_async_request_t* const req) {

        assert(req != NULL);
        assert(hx711_multi__is_state_machines_enabled(req->_hxm));

        //if already reading, don't start again
        assert(req->_state == HX711_MULTI_ASYNC_STATE_WAITING);

        UTIL_INTERRUPTS_OFF_BLOCK(

            util_pio_sm_clear_rx_fifo(
                req->_hxm->_pio,
                req->_hxm->_reader_sm);

            //listen for DMA done
            dma_irqn_set_channel_enabled(
                req->dma_irq_index,
                req->_hxm->_dma_channel,
                true);

            req->_state = HX711_MULTI_ASYNC_STATE_READING;

            dma_channel_set_write_addr(
                req->_hxm->_dma_channel,
                req->_buffer,
                true); //trigger

        );

}

void __isr __not_in_flash_func(hx711_multi__async_pio_irq_handler)() {

    hx711_multi_async_request_t* const req = 
        hx711_multi__async_get_pio_irq_request();

    assert(req != NULL);
    assert(hx711_multi__is_state_machines_enabled(req->_hxm));
    assert(req->_state == HX711_MULTI_ASYNC_STATE_WAITING);

    hx711_multi__async_start_dma(req);

    //don't need to clear the PIO interrupt, but
    //just disable listening for it again until required
    pio_set_irqn_source_enabled(
        req->_hxm->_pio,
        req->pio_irq_index,
        util_pio_get_pis(HX711_MULTI_CONVERSION_DONE_IRQ_NUM),
        false);

    irq_clear(
        util_pion_get_irqn(
            req->_hxm->_pio,
            req->pio_irq_index));

}

void __isr __not_in_flash_func(hx711_multi__async_dma_irq_handler)() {

    hx711_multi_async_request_t* const req =
        hx711_multi__async_get_dma_irq_request();

    assert(req != NULL);
    assert(hx711_multi__is_state_machines_enabled(req->_hxm));
    assert(req->_state == HX711_MULTI_ASYNC_STATE_READING);

    req->_state = HX711_MULTI_ASYNC_STATE_DONE;

    //don't need to listen for dma irqs again until required
    dma_irqn_set_channel_enabled(
        req->dma_irq_index,
        req->_hxm->_dma_channel,
        false);

    dma_irqn_acknowledge_channel(
        req->dma_irq_index,
        req->_hxm->_dma_channel);

    irq_clear(
        util_dma_get_irqn(
            req->dma_irq_index));

}

bool hx711_multi__async_add_request(
    hx711_multi_async_request_t* const req) {

        assert(req != NULL);
        assert(hx711_multi__is_state_machines_enabled(req->_hxm));

        for(uint i = 0; i < HX711_MULTI_ASYNC_REQ_COUNT; ++i) {
            if(hx711_multi__async_request_map[i] == NULL) {
                hx711_multi__async_request_map[i] = req;
                return true;
            }
        }

        return false;

}

void hx711_multi__async_remove_request(
    const hx711_multi_async_request_t* const req) {

        assert(req != NULL);
        assert(hx711_multi__is_state_machines_enabled(req->_hxm));

        for(uint i = 0; i < HX711_MULTI_ASYNC_REQ_COUNT; ++i) {
            if(hx711_multi__async_request_map[i] == req) {
                hx711_multi__async_request_map[i] = NULL;
                return;
            }
        }

}

void hx711_multi_async_get_request_defaults(
    hx711_multi_t* const hxm,
    hx711_multi_async_request_t* const req) {

        assert(hx711_multi__is_state_machines_enabled(req->_hxm));
        assert(req != NULL);

        req->_hxm = hxm;
        req->pio_irq_index = HX711_MULTI_ASYNC_PIO_IRQ_IDX;
        req->dma_irq_index = HX711_MULTI_ASYNC_DMA_IRQ_IDX;
        req->_state = HX711_MULTI_ASYNC_STATE_NONE;

}

void hx711_multi_async_open(
    hx711_multi_t* const hxm,
    hx711_multi_async_request_t* const req) {

        assert(hx711_multi__is_state_machines_enabled(hxm));
        assert(req != NULL);
        assert(req->_buffer != NULL);
        assert(util_pio_irq_index_is_valid(req->pio_irq_index));
        assert(util_dma_irq_index_is_valid(req->dma_irq_index));

        mutex_enter_blocking(&hxm->_mut);

        req->_state = HX711_MULTI_ASYNC_STATE_NONE;

        hx711_multi__async_add_request(req);

        //enable irqs for dma transfers complete
        util_dma_channel_set_quiet(
            hxm->_dma_channel,
            false);

        //make sure it is DISABLED at this point!
        dma_irqn_set_channel_enabled(
            req->dma_irq_index,
            hxm->_dma_channel,
            false);

        irq_set_exclusive_handler(
            util_dma_get_irqn(req->dma_irq_index),
            hx711_multi__async_dma_irq_handler);

        //route dma irq to NVIC irq
        irq_set_enabled(
            util_dma_get_irqn(req->dma_irq_index),
            true);

        //make sure it is DISABLED at this point!
        pio_set_irqn_source_enabled(
            hxm->_pio,
            req->pio_irq_index,
            util_pio_get_pis(HX711_MULTI_CONVERSION_DONE_IRQ_NUM),
            false);

        irq_set_exclusive_handler(
            util_pion_get_irqn(hxm->_pio, req->pio_irq_index),
            hx711_multi__async_pio_irq_handler);

        //route pio irq to NVIC irq
        irq_set_enabled(
            util_pion_get_irqn(
                hxm->_pio,
                req->pio_irq_index),
            true);

}

void hx711_multi_async_start(
    hx711_multi_async_request_t* const req) {

        assert(req != NULL);

        req->_state = HX711_MULTI_ASYNC_STATE_WAITING;

        //if pio interrupt is already set, we can bypass the
        //IRQ handler and immediately trigger dma
        if(pio_interrupt_get(req->_hxm->_pio, HX711_MULTI_CONVERSION_DONE_IRQ_NUM)) {
            hx711_multi__async_start_dma(req);
        }
        else {
            pio_set_irqn_source_enabled(
                req->_hxm->_pio,
                req->pio_irq_index,
                util_pion_get_irqn(
                    req->_hxm->_pio,
                    req->pio_irq_index),
                true);
        }

}

bool hx711_multi_async_is_done(
    hx711_multi_async_request_t* const req) {
        assert(req != NULL);
        return req->_state == HX711_MULTI_ASYNC_STATE_DONE;
}

void hx711_multi_async_get_values(
    hx711_multi_async_request_t* const req,
    int32_t* const values) {

        assert(req != NULL);
        assert(values != NULL);
        assert(hx711_multi_async_is_done(req));

        hx711_multi_pinvals_to_values(
            req->_buffer,
            values,
            req->_hxm->_chips_len);

        req->_state = HX711_MULTI_ASYNC_STATE_NONE;

}

void hx711_multi_async_close(
    hx711_multi_t* const hxm,
    hx711_multi_async_request_t* const req) {

        assert(hxm != NULL);
        assert(req != NULL);

        UTIL_INTERRUPTS_OFF_BLOCK(

            dma_channel_abort(hxm->_dma_channel);

            pio_set_irqn_source_enabled(
                hxm->_pio,
                req->pio_irq_index,
                util_pio_get_pis(HX711_MULTI_CONVERSION_DONE_IRQ_NUM),
                false);

            irq_set_enabled(
                util_pion_get_irqn(
                    hxm->_pio,
                    req->pio_irq_index),
                false);

            irq_set_enabled(
                util_dma_get_irqn(req->dma_irq_index),
                false);

            irq_remove_handler(
                util_pion_get_irqn(
                    hxm->_pio,
                    req->pio_irq_index),
                hx711_multi__async_pio_irq_handler);

            irq_remove_handler(
                util_dma_get_irqn(req->dma_irq_index),
                hx711_multi__async_pio_irq_handler);

            util_dma_channel_set_quiet(
                hxm->_dma_channel,
                true);

            hx711_multi__async_remove_request(req);

            req->_state = HX711_MULTI_ASYNC_STATE_NONE;

        );

        mutex_exit(&hxm->_mut);

}

void hx711_multi_power_up(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        assert(hx711_multi__is_initd(hxm));

        const uint32_t gainVal = hx711_gain_to_pio_gain(gain);

        assert(hx711_is_pio_gain_valid(gainVal));

        UTIL_MUTEX_BLOCK(hxm->_mut, 

            gpio_put(
                hxm->_clock_pin,
                false);

            pio_sm_init(
                hxm->_pio,
                hxm->_reader_sm,
                hxm->_reader_offset,
                &hxm->_reader_default_config);

            pio_sm_clear_fifos(
                hxm->_pio,
                hxm->_reader_sm);

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

            pio_set_sm_mask_enabled(
                hxm->_pio,
                (1 << hxm->_awaiter_sm) | (1 << hxm->_reader_sm),
                true);

        );

}

void hx711_multi_power_down(hx711_multi_t* const hxm) {

    assert(hx711_multi__is_initd(hxm));

    UTIL_MUTEX_BLOCK(hxm->_mut,

        dma_channel_abort(hxm->_dma_channel);

        pio_set_sm_mask_enabled(
            hxm->_pio,
            (1 << hxm->_awaiter_sm) | (1 << hxm->_reader_sm),
            false);

        gpio_put(
            hxm->_clock_pin,
            true);

    );

}

void hx711_multi_sync(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {
        assert(hx711_multi__is_initd(hxm));
        hx711_multi_power_down(hxm);
        hx711_wait_power_down();
        hx711_multi_power_up(hxm, gain);
}

uint32_t hx711_multi_sync_state(
    hx711_multi_t* const hxm) {
        assert(hx711_multi__is_state_machines_enabled(hxm));
        return pio_sm_get_blocking(hxm->_pio, hxm->_awaiter_sm);
}

bool hx711_multi_is_syncd(
    hx711_multi_t* const hxm) {

        assert(hx711_multi__is_state_machines_enabled(hxm));

        //all chips should either be 0 or 1 which translates
        //to a bitmask of exactly 0 or 2^chips
        const uint32_t allReady = (uint32_t)pow(2, hxm->_chips_len);
        const uint32_t state = hx711_multi_sync_state(hxm);

        return state == 0 || state == allReady;

}