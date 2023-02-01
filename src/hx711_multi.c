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

        mutex_init(&hxm->_mut);

        UTIL_MUTEX_BLOCK(hxm->_mut, 

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
                    hxm->_reader_sm, false));

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
            hxm->_awaiter_sm | hxm->_reader_sm,
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

        //no need to 0-init this, pinvals are ignored in this func
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
                hxm->_awaiter_sm | hxm->_reader_sm,
                true);

        )

}

void hx711_multi_power_down(hx711_multi_t* const hxm) {

    HX711_MULTI_ASSERT_INITD(hxm)

    UTIL_MUTEX_BLOCK(hxm->_mut,

        dma_channel_abort(hxm->_dma_channel);

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

    )

}

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
        util_pio_interrupt_wait_cleared(
            hxm->_pio,
            HX711_MULTI_CONVERSION_RUNNING_IRQ_NUM);

        //clear any residual data
        util_pio_sm_clear_rx_fifo(
            hxm->_pio,
            hxm->_reader_sm);

        //at this stage there is no need to wait for the
        //conversion period irq to be set, just let DMA
        //handle the writes when they begin

        //then start reading
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

        util_pio_interrupt_wait_cleared_timeout(
            hxm->_pio,
            HX711_MULTI_CONVERSION_RUNNING_IRQ_NUM,
            end);

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
        else {
            assert(util_dma_get_transfer_count(hxm->_dma_channel) == 0);
            assert(pio_sm_is_rx_fifo_empty(hxm->_pio, hxm->_reader_sm));
        }

        return isDone;

}
