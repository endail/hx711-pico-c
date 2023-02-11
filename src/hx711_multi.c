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
#include "hardware/irq.h"

volatile hx711_multi_async_request_t* volatile hx711_multi__async_request_map[] = {
    NULL,
    NULL
};

void hx711_multi__pinvals_to_values(
    const uint32_t* const pinvals,
    int32_t* const values,
    const size_t len) {

        UTIL_ASSERT_NOT_NULL(pinvals)
        UTIL_ASSERT_NOT_NULL(values)
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

            HX711_ASSERT_VALUE(values[chipNum]);

        }

}

void hx711_multi__get_values_raw(
    hx711_multi_t* const hxm,
    uint32_t* const pinvals) {

        HX711_MULTI_ASSERT_INITD(hxm)
        HX711_MULTI_ASSERT_STATE_MACHINES_ENABLED(hxm)
        UTIL_ASSERT_NOT_NULL(pinvals)

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

        HX711_MULTI_ASSERT_INITD(hxm)
        HX711_MULTI_ASSERT_STATE_MACHINES_ENABLED(hxm)
        UTIL_ASSERT_NOT_NULL(pinvals)
        UTIL_ASSERT_NOT_NULL(end)
        assert(!is_nil_time(*end));
        assert(!dma_channel_is_busy(hxm->_dma_channel));

        //wait for any current conversion period to end to
        //be able to sync with the next period
        if(!util_pio_interrupt_wait_timeout(
            hxm->_pio,
            HX711_MULTI_CONVERSION_RUNNING_IRQ_NUM,
            end)) {
                //failed to reach conversion period within
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

        UTIL_ASSERT_NOT_NULL(hxm)
        UTIL_ASSERT_NOT_NULL(config)

        UTIL_ASSERT_RANGE(
            config->chips_len,
            HX711_MULTI_MIN_CHIPS,
            HX711_MULTI_MAX_CHIPS)

        UTIL_ASSERT_NOT_NULL(config->pio)
        UTIL_ASSERT_NOT_NULL(config->pio_init)

        UTIL_ASSERT_NOT_NULL(config->awaiter_prog)
        UTIL_ASSERT_NOT_NULL(config->awaiter_prog_init)

        UTIL_ASSERT_NOT_NULL(config->reader_prog)
        UTIL_ASSERT_NOT_NULL(config->reader_prog_init)

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

        )

}

void hx711_multi_close(hx711_multi_t* const hxm) {

    HX711_MULTI_ASSERT_INITD(hxm)

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

    )

}

void hx711_multi_set_gain(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        HX711_MULTI_ASSERT_INITD(hxm)
        HX711_MULTI_ASSERT_STATE_MACHINES_ENABLED(hxm)

        uint32_t dummy[HX711_READ_BITS];
        const uint32_t gainVal = hx711__gain_to_pio_gain(gain);

        HX711_ASSERT_PIO_GAIN(gainVal)

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

        )

}

void hx711_multi_get_values(
    hx711_multi_t* const hxm,
    int32_t* const values) {

        HX711_MULTI_ASSERT_INITD(hxm)
        HX711_MULTI_ASSERT_STATE_MACHINES_ENABLED(hxm)
        UTIL_ASSERT_NOT_NULL(values)

        uint32_t pinvals[HX711_READ_BITS];

        UTIL_MUTEX_BLOCK(hxm->_mut, 
            hx711_multi__get_values_raw(
                hxm,
                pinvals);
        )

        hx711_multi__pinvals_to_values(
            pinvals,
            values,
            hxm->_chips_len);

}

bool hx711_multi_get_values_timeout(
    hx711_multi_t* const hxm,
    int32_t* const values,
    const uint timeout) {

        HX711_MULTI_ASSERT_INITD(hxm)
        HX711_MULTI_ASSERT_STATE_MACHINES_ENABLED(hxm)
        UTIL_ASSERT_NOT_NULL(values)

        uint32_t pinvals[HX711_READ_BITS];
        const absolute_time_t end = make_timeout_time_us(timeout);

        bool success;

        UTIL_MUTEX_BLOCK(hxm->_mut, 
            success = hx711_multi__get_values_timeout_raw(
                hxm,
                pinvals,
                &end);
        )

        if(success) {
            hx711_multi__pinvals_to_values(
                pinvals,
                values,
                hxm->_chips_len);
        }

        return success;

}

bool hx711_multi__async_pio_irq_is_set(
    volatile hx711_multi_async_request_t* volatile const req) {
        UTIL_ASSERT_NOT_NULL(req)
        //return (req->_pio->irq & HX711_MULTI_CONVERSION_DONE_IRQ_NUM) != 0;
        return pio_interrupt_get(req->_pio, HX711_MULTI_CONVERSION_DONE_IRQ_NUM);
}

bool hx711_multi__async_dma_irq_is_set(
    volatile hx711_multi_async_request_t* volatile const req) {
        UTIL_ASSERT_NOT_NULL(req)
        return dma_irqn_get_channel_status(req->dma_irq_index, req->_channel);
}

volatile hx711_multi_async_request_t* volatile hx711_multi__async_get_dma_irq_request() {

    for(uint i = 0; i < NUM_PIOS; ++i) {

        if(hx711_multi__async_request_map[i] == NULL) {
            continue;
        }

        if(hx711_multi__async_dma_irq_is_set(hx711_multi__async_request_map[i])) {
            return hx711_multi__async_request_map[i];
        }

    }

    return NULL;

}

volatile hx711_multi_async_request_t* volatile hx711_multi__async_get_pio_irq_request() {

    for(uint i = 0; i < NUM_PIOS; ++i) {

        if(hx711_multi__async_request_map[i] == NULL) {
            continue;
        }

        if(hx711_multi__async_pio_irq_is_set(hx711_multi__async_request_map[i])) {
            return hx711_multi__async_request_map[i];
        }

    }

    return NULL;

}

void __isr __not_in_flash_func(hx711_multi__async_pio_irq_handler)() {

    volatile hx711_multi_async_request_t* volatile req = 
        hx711_multi__async_get_pio_irq_request();

    UTIL_ASSERT_NOT_NULL(req)
    assert(req->_state == HX711_MULTI_ASYNC_STATE_WAITING);

    hx711_multi__async_start_dma(req);
        
    //pio irq not needed any more
    //irq_set_enabled(
    //    util_pion_get_irqn(req->_pio, req->pio_irq_index),
    //    false);

    irq_clear(
        util_pion_get_irqn(
            req->_pio,
            req->pio_irq_index));

}

void __isr __not_in_flash_func(hx711_multi__async_dma_irq_handler)() {

    volatile hx711_multi_async_request_t* volatile req =
        hx711_multi__async_get_dma_irq_request();

    UTIL_ASSERT_NOT_NULL(req)
    //assert(req->_state == HX711_MULTI_ASYNC_STATE_READING);

    req->_state = HX711_MULTI_ASYNC_STATE_DONE;

    //dma no longer needed
    //irq_set_enabled(
    //    util_dma_get_irqn(req->dma_irq_index),
    //    false);

    dma_irqn_acknowledge_channel(
        req->dma_irq_index,
        req->_channel);

    irq_clear(
        util_dma_get_irqn(
            req->dma_irq_index));

}

void hx711_multi__async_start_dma(
    volatile hx711_multi_async_request_t* volatile const req) {

        UTIL_ASSERT_NOT_NULL(req)

        //if already reading, don't start again
        assert(req->_state == HX711_MULTI_ASYNC_STATE_WAITING);

        //UTIL_INTERRUPTS_OFF_BLOCK(

            util_pio_sm_clear_rx_fifo(
                req->_pio,
                req->_sm);

            dma_channel_set_write_addr(
                req->_channel,
                req->_buffer,
                true);

            req->_state = HX711_MULTI_ASYNC_STATE_READING;

            //listen for DMA done
            //irq_set_enabled(
            //    util_dma_get_irqn(req->dma_irq_index),
            //    true);

        //)

}

bool hx711_multi__async_set_free_map_location(
    volatile hx711_multi_async_request_t* volatile const req) {

        for(uint i = 0; i < NUM_PIOS; ++i) {
            if(hx711_multi__async_request_map[i] == NULL) {
                hx711_multi__async_request_map[i] = req;
                return true;
            }
        }

        return false;

}

void hx711_multi_async_get_request_defaults(
    hx711_multi_t* const hxm,
    hx711_multi_async_request_t* const req) {
        req->pio_irq_index = HX711_MULTI_ASYNC_PIO_IRQ_IDX;
        req->dma_irq_index = HX711_MULTI_ASYNC_DMA_IRQ_IDX;
        req->_state = HX711_MULTI_ASYNC_STATE_NONE;
        req->_pio = hxm->_pio;
        req->_channel = hxm->_dma_channel;
        req->_sm = hxm->_reader_sm;
        req->_buff_len = hxm->_chips_len;
}

void hx711_multi_async_open(
    hx711_multi_t* const hxm,
    hx711_multi_async_request_t* const req) {

        UTIL_ASSERT_NOT_NULL(hxm)
        UTIL_ASSERT_NOT_NULL(req)
        HX711_MULTI_ASSERT_STATE_MACHINES_ENABLED(hxm)

        mutex_enter_blocking(&hxm->_mut);

        req->_state = HX711_MULTI_ASYNC_STATE_NONE;

        hx711_multi__async_set_free_map_location(req);

        dma_channel_config cfg = dma_get_channel_config(hxm->_dma_channel);
        channel_config_set_irq_quiet(&cfg, false);
        dma_channel_set_config(hxm->_dma_channel, &cfg, false);

        dma_irqn_set_channel_enabled(
            req->dma_irq_index,
            req->_channel,
            true);

        irq_set_exclusive_handler(
            util_dma_get_irqn(req->dma_irq_index),
            hx711_multi__async_dma_irq_handler);

        pio_set_irqn_source_enabled(
            hxm->_pio,
            req->pio_irq_index,
            util_pio_get_pis(HX711_MULTI_CONVERSION_DONE_IRQ_NUM),
            true);

        irq_set_exclusive_handler(
            util_pion_get_irqn(hxm->_pio, req->pio_irq_index),
            hx711_multi__async_pio_irq_handler);

}

void hx711_multi_async_start(
    hx711_multi_async_request_t* const req) {

        UTIL_ASSERT_NOT_NULL(req)

        //UTIL_INTERRUPTS_OFF_BLOCK(

            //always reset the write pointer
            req->_state = HX711_MULTI_ASYNC_STATE_WAITING;

            irq_set_enabled(
                util_dma_get_irqn(req->dma_irq_index),
                true);

            if(pio_interrupt_get(req->_pio, HX711_MULTI_CONVERSION_DONE_IRQ_NUM)) {
                hx711_multi__async_start_dma(req);
            }
            else {
                irq_set_enabled(
                    util_pion_get_irqn(
                        req->_pio,
                        req->pio_irq_index),
                    true);
            }

        //)

}

bool hx711_multi_async_is_done(
    hx711_multi_async_request_t* const req) {

        UTIL_ASSERT_NOT_NULL(req)

        //return !dma_channel_is_busy(req->_channel);

        return req->_state == HX711_MULTI_ASYNC_STATE_DONE;

/*
        if(req->_state == HX711_MULTI_ASYNC_STATE_DONE) {
            return true;
        }

        if(req->_state == HX711_MULTI_ASYNC_STATE_READING) {
            if(!dma_channel_is_busy(req->_channel)) {
                req->_state = HX711_MULTI_ASYNC_STATE_DONE;
                irq_set_enabled(
                    util_pion_get_irqn(req->_pio, req->pio_irq_index),
                    false);
                return true;
            }
        }

        return false;
*/

}

void hx711_multi_async_get_values(
    hx711_multi_async_request_t* const req,
    int32_t* const values) {

        UTIL_ASSERT_NOT_NULL(req)
        UTIL_ASSERT_NOT_NULL(values)
        assert(hx711_multi_async_is_done(req));

        hx711_multi__pinvals_to_values(
            (uint32_t*)req->_buffer,
            values,
            req->_buff_len);

        req->_state = HX711_MULTI_ASYNC_STATE_NONE;

}

void hx711_multi_async_close(
    hx711_multi_t* const hxm,
    hx711_multi_async_request_t* const req) {

        UTIL_ASSERT_NOT_NULL(hxm)
        UTIL_ASSERT_NOT_NULL(req)

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

            dma_channel_config cfg = dma_get_channel_config(hxm->_dma_channel);
            channel_config_set_irq_quiet(&cfg, true);
            dma_channel_set_config(hxm->_dma_channel, &cfg, false);

            hx711_multi__async_request_map[pio_get_index(hxm->_pio)] = NULL;

            req->_state = HX711_MULTI_ASYNC_STATE_NONE;

        )

        mutex_exit(&hxm->_mut);

}

void hx711_multi_power_up(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        HX711_MULTI_ASSERT_INITD(hxm)

        const uint32_t gainVal = hx711__gain_to_pio_gain(gain);

        HX711_ASSERT_PIO_GAIN(gainVal)

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

        )

}

void hx711_multi_power_down(hx711_multi_t* const hxm) {

    HX711_MULTI_ASSERT_INITD(hxm)

    UTIL_MUTEX_BLOCK(hxm->_mut,

        dma_channel_abort(hxm->_dma_channel);

        pio_set_sm_mask_enabled(
            hxm->_pio,
            (1 << hxm->_awaiter_sm) | (1 << hxm->_reader_sm),
            false);

        gpio_put(
            hxm->_clock_pin,
            true);

    )

}

void hx711_multi_sync(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {
        HX711_MULTI_ASSERT_INITD(hxm)
        hx711_multi_power_down(hxm);
        hx711_wait_power_down();
        hx711_multi_power_up(hxm, gain);
}
