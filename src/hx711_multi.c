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

hx711_multi_t* hx711_multi__async_read_array[] = {
    NULL, //...
};

void hx711_multi__init_asert(
    const hx711_multi_config_t* const config) {

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

        assert(util_pio_irq_index_is_valid(config->pio_irq_index));
        assert(util_dma_irq_index_is_valid(config->dma_irq_index));

#ifndef NDEBUG
        {
            //make sure none of the data pins are also the clock pin
            const uint l = config->data_pin_base + config->chips_len - 1;
            for(uint i = config->data_pin_base; i <= l; ++i) {
                assert(i != config->clock_pin);
            }
        }
#endif

}

void hx711_multi__init_pio(hx711_multi_t* const hxm) {

    //adding programs and claiming state machines
    //will panic if unable; this is appropriate.
    hxm->_awaiter_offset = pio_add_program(
        hxm->_pio,
        hxm->_awaiter_prog);

    hxm->_reader_offset = pio_add_program(
        hxm->_pio,
        hxm->_reader_prog);

    /**
     * Casting pio_claim_unused_sm to uint is OK in this
     * circumstance. Ordinarily it would return -1 if the
     * claim failed, but since the flag is given to require
     * a PIO State Machine, panic would be called instead.
     */

    hxm->_awaiter_sm = (uint)pio_claim_unused_sm(
        hxm->_pio,
        true);

    hxm->_reader_sm = (uint)pio_claim_unused_sm(
        hxm->_pio,
        true);

}

void hx711_multi__init_dma(hx711_multi_t* const hxm) {

    /**
     * Casting dma_claim_unused_channel to uint is OK in this
     * circumstance. Ordinarily it would return -1 if the
     * claim failed, but since the flag is given to require
     * a DMA channel, panic would be called instead.
     */
    hxm->_dma_channel = (uint)dma_claim_unused_channel(true);

    dma_channel_config cfg = dma_channel_get_default_config(
        hxm->_dma_channel);

    /**
     * Do not set ring buffer.
     * ie. do not use channel_config_set_ring.
     * If, for whatever reason, the DMA transfer
     * fails, subsequent transfer invocations
     * will reset the write address.
     */

    /**
     * PIO RX FIFO output is 32 bits, so the DMA read needs
     * to match or be larger.
     */
    channel_config_set_transfer_data_size(
        &cfg,
        DMA_SIZE_32);

    /**
     * DMA is always going to read from the same location,
     * which is the PIO FIFO.
     */
    channel_config_set_read_increment(
        &cfg,
        false);

    /**
     * Each successive read from the PIO RX FIFO needs to be
     * to the next array buffer position.
     */
    channel_config_set_write_increment(
        &cfg,
        true);

    /**
     * DMA transfers are paced based on the PIO RX DREQ.
     */
    channel_config_set_dreq(
        &cfg,
        pio_get_dreq(
            hxm->_pio,
            hxm->_reader_sm,
            false));

    /**
     * Quiet needs to be disabled in order for DMA to raise
     * an interrupt when each transfer is complete. This is
     * necessary for this implementation to work.
     */
    channel_config_set_irq_quiet(
        &cfg,
        false);

    dma_channel_configure(
        hxm->_dma_channel,
        &cfg,
        NULL,                               //don't set a write address yet
        &hxm->_pio->rxf[hxm->_reader_sm],   //read from reader pio program rx fifo
        HX711_READ_BITS,                    //24 transfers; one for each HX711 bit
        false);                             //false = don't start now

}

void hx711_multi__init_irq(hx711_multi_t* const hxm) {

    /**
     * The idea here is that the PIO and DMA IRQs can be
     * set up, enabled, and routed out to NVIC IRQs and
     * be enabled at this point. If and when IRQs need to
     * be disabled, they can be done at the source BEFORE
     * being routed out to NVIC and triggering a
     * system-wide interrupt.
     */

    /**
     * DMA interrupts can always remain enabled. They will
     * only trigger following a PIO interrupt.
     */
    dma_irqn_set_channel_enabled(
        hxm->_dma_irq_index,
        hxm->_dma_channel,
        true);

    irq_set_exclusive_handler(
        util_dma_get_irqn(hxm->_dma_irq_index),
        hx711_multi__async_dma_irq_handler);

    irq_set_enabled(
        util_dma_get_irqn(hxm->_dma_irq_index),
        true);

    /**
     * The PIO source interrupt MUST REMAIN DISABLED
     * until the point at which it is required to listen
     * to them.
     */
    pio_set_irqn_source_enabled(
        hxm->_pio,
        hxm->_pio_irq_index,
        util_pio_get_pis_from_pio_interrupt_num(
            HX711_MULTI_CONVERSION_DONE_IRQ_NUM),
        false);

    irq_set_exclusive_handler(
        util_pio_get_irq_from_index(hxm->_pio, hxm->_pio_irq_index),
        hx711_multi__async_pio_irq_handler);

    irq_set_enabled(
        util_pio_get_irq_from_index(hxm->_pio, hxm->_pio_irq_index),
        true);

}

bool hx711_multi__async_dma_irq_is_set(
    hx711_multi_t* const hxm) {

        assert(hx711_multi__is_initd(hxm));

        return dma_irqn_get_channel_status(
            hxm->_dma_irq_index,
            hxm->_dma_channel);

}

bool hx711_multi__async_pio_irq_is_set(
    hx711_multi_t* const hxm) {

        assert(hx711_multi__is_initd(hxm));

        return pio_interrupt_get(
            hxm->_pio,
            HX711_MULTI_CONVERSION_DONE_IRQ_NUM);

}

hx711_multi_t* const hx711_multi__async_get_dma_irq_request() {

    assert(hx711_multi__async_read_array != NULL);

    for(uint i = 0; i < HX711_MULTI_ASYNC_READ_COUNT; ++i) {

        if(hx711_multi__async_read_array[i] == NULL) {
            continue;
        }
        
        if(hx711_multi__async_dma_irq_is_set(hx711_multi__async_read_array[i])) {
            return hx711_multi__async_read_array[i];
        }

    }

    return NULL;

}

hx711_multi_t* const hx711_multi__async_get_pio_irq_request() {

    assert(hx711_multi__async_read_array != NULL);

    for(uint i = 0; i < HX711_MULTI_ASYNC_READ_COUNT; ++i) {

        if(hx711_multi__async_read_array[i] == NULL) {
            continue;
        }

        if(hx711_multi__async_pio_irq_is_set(hx711_multi__async_read_array[i])) {
            return hx711_multi__async_read_array[i];
        }

    }

    return NULL;

}

void hx711_multi__async_start_dma(
    hx711_multi_t* const hxm) {

        assert(hx711_multi__is_state_machines_enabled(hxm));
        assert(hxm->_async_state == HX711_MULTI_ASYNC_STATE_WAITING);

        util_pio_sm_clear_rx_fifo(
            hxm->_pio,
            hxm->_reader_sm);

        //listen for DMA done
        dma_irqn_set_channel_enabled(
            hxm->_dma_irq_index,
            hxm->_dma_channel,
            true);

        hxm->_async_state = HX711_MULTI_ASYNC_STATE_READING;

        dma_channel_set_write_addr(
            hxm->_dma_channel,
            hxm->_buffer,
            true); //trigger

}

bool hx711_multi__async_is_running(
    hx711_multi_t* const hxm) {

        assert(hx711_multi__is_state_machines_enabled(hxm));

        switch(hxm->_async_state) {
            case HX711_MULTI_ASYNC_STATE_WAITING:
            case HX711_MULTI_ASYNC_STATE_READING:
                return true;
            default:
                //anything else is not considered running
                return false;
        }

}

static void hx711_multi__async_finish(
    hx711_multi_t* const hxm) {

        assert(hx711_multi__is_initd(hxm));

        //stop listening for IRQs

        dma_channel_abort(hxm->_dma_channel);

        dma_irqn_set_channel_enabled(
            hxm->_dma_irq_index,
            hxm->_dma_channel,
            false);

        pio_set_irqn_source_enabled(
            hxm->_pio,
            hxm->_pio_irq_index,
            util_pio_get_pis_from_pio_interrupt_num(HX711_MULTI_CONVERSION_DONE_IRQ_NUM),
            false);

#ifndef HX711_NO_MUTEX
        mutex_exit(&hxm->_mut);
#endif

}

void __isr __not_in_flash_func(hx711_multi__async_pio_irq_handler)() {

    hx711_multi_t* const hxm = 
        hx711_multi__async_get_pio_irq_request();

    assert(hx711_multi__is_state_machines_enabled(hxm));
    assert(hxm->_async_state == HX711_MULTI_ASYNC_STATE_WAITING);

    hx711_multi__async_start_dma(hxm);

    //disable listening until required again
    pio_set_irqn_source_enabled(
        hxm->_pio,
        hxm->_pio_irq_index,
        util_pio_get_pis_from_pio_interrupt_num(HX711_MULTI_CONVERSION_DONE_IRQ_NUM),
        false);

    irq_clear(
        util_pio_get_irq_from_index(hxm->_pio, hxm->_pio_irq_index));

}

void __isr __not_in_flash_func(hx711_multi__async_dma_irq_handler)() {

    hx711_multi_t* const hxm =
        hx711_multi__async_get_dma_irq_request();

    assert(hx711_multi__is_state_machines_enabled(hxm));
    assert(hxm->_async_state == HX711_MULTI_ASYNC_STATE_READING);

    hxm->_async_state = HX711_MULTI_ASYNC_STATE_DONE;

    dma_irqn_acknowledge_channel(
        hxm->_dma_irq_index,
        hxm->_dma_channel);

    hx711_multi__async_finish(hxm);

    irq_clear(
        util_dma_get_irqn(
            hxm->_dma_irq_index));

}

bool hx711_multi__async_add_reader(
    hx711_multi_t* const hxm) {

        assert(hx711_multi__async_read_array != NULL);

        for(uint i = 0; i < HX711_MULTI_ASYNC_READ_COUNT; ++i) {
            if(hx711_multi__async_read_array[i] == NULL) {
                hx711_multi__async_read_array[i] = hxm;
                return true;
            }
        }

        return false;

}

void hx711_multi__async_remove_reader(
    const hx711_multi_t* const hxm) {

        //we don't care whether it's initd at this point
        //or whether the SMs are running; just remove it
        //from the array
        assert(hxm != NULL);
        assert(hx711_multi__async_read_array != NULL);

        for(uint i = 0; i < HX711_MULTI_ASYNC_READ_COUNT; ++i) {
            if(hx711_multi__async_read_array[i] == hxm) {
                hx711_multi__async_read_array[i] = NULL;
                return;
            }
        }

}

static bool hx711_multi__is_initd(hx711_multi_t* const hxm) {
    return hxm != NULL &&
        hxm->_pio != NULL &&
        pio_sm_is_claimed(hxm->_pio, hxm->_awaiter_sm) &&
        pio_sm_is_claimed(hxm->_pio, hxm->_reader_sm) &&
        dma_channel_is_claimed(hxm->_dma_channel) &&
#ifndef HX711_NO_MUTEX
        mutex_is_initialized(&hxm->_mut) &&
#endif
        irq_get_exclusive_handler(util_pio_get_irq_from_index(
            hxm->_pio,
            hxm->_pio_irq_index)) == hx711_multi__async_pio_irq_handler &&
        irq_get_exclusive_handler(util_dma_get_irqn(
            hxm->_dma_irq_index)) == hx711_multi__async_dma_irq_handler;
}

static bool hx711_multi__is_state_machines_enabled(
    hx711_multi_t* const hxm) {
        return hx711_multi__is_initd(hxm) &&
            util_pio_sm_is_enabled(hxm->_pio, hxm->_awaiter_sm) &&
            util_pio_sm_is_enabled(hxm->_pio, hxm->_reader_sm);
}

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

void hx711_multi_init(
    hx711_multi_t* const hxm,
    const hx711_multi_config_t* const config) {

        hx711_multi__init_asert(config);

#ifndef HX711_NO_MUTEX
        //this doesn't need to be an interrupts-off block
        //because the mutex will protect the hxm from reads
        //until init returns, and the source IRQs aren't
        //enabled anyway
        mutex_init(&hxm->_mut);
#endif

        HX711_MUTEX_BLOCK(hxm->_mut, 

            hxm->_clock_pin = config->clock_pin;
            hxm->_data_pin_base = config->data_pin_base;
            hxm->_chips_len = config->chips_len;

            hxm->_pio = config->pio;
            hxm->_awaiter_prog = config->awaiter_prog;
            hxm->_reader_prog = config->reader_prog;

            hxm->_pio_irq_index = config->pio_irq_index;
            hxm->_dma_irq_index = config->dma_irq_index;

            hxm->_async_state = HX711_MULTI_ASYNC_STATE_NONE;

            hx711_multi__async_add_reader(hxm);

            util_gpio_set_output(hxm->_clock_pin);

            util_gpio_set_contiguous_input_pins(
                hxm->_data_pin_base,
                hxm->_chips_len);

            hx711_multi__init_pio(hxm);

            config->pio_init(hxm);
            config->awaiter_prog_init(hxm);
            config->reader_prog_init(hxm);

            hx711_multi__init_dma(hxm);
            hx711_multi__init_irq(hxm);

        );

}

void hx711_multi_close(hx711_multi_t* const hxm) {

    assert(hx711_multi__is_initd(hxm));

#ifndef HX711_NO_MUTEX
    mutex_enter_blocking(&hxm->_mut);
#endif

    //make sure the disabling and removal of IRQs and
    //handlers is atomic
    UTIL_INTERRUPTS_OFF_BLOCK(

        //interrupts are off, but cancel any running
        //async reads
        dma_channel_abort(hxm->_dma_channel);

        irq_set_enabled(
            util_pio_get_irq_from_index(hxm->_pio, hxm->_pio_irq_index),
            false);

        irq_set_enabled(
            util_dma_get_irqn(hxm->_dma_irq_index),
            false);

        pio_set_irqn_source_enabled(
            hxm->_pio,
            hxm->_pio_irq_index,
            util_pio_get_pis_from_pio_interrupt_num(HX711_MULTI_CONVERSION_DONE_IRQ_NUM),
            false);

        dma_irqn_set_channel_enabled(
            hxm->_dma_irq_index,
            hxm->_dma_channel,
            false);

        hxm->_async_state = HX711_MULTI_ASYNC_STATE_NONE;

        hx711_multi__async_remove_reader(hxm);

        irq_remove_handler(
            util_pio_get_irq_from_index(hxm->_pio, hxm->_pio_irq_index),
            hx711_multi__async_pio_irq_handler);

        irq_remove_handler(
            util_dma_get_irqn(hxm->_dma_irq_index),
            hx711_multi__async_pio_irq_handler);

    );

    //at this point it is impossible for a relevant DMA
    //or PIO IRQ to occur, so we can turn interrupts
    //back on

    util_dma_channel_set_quiet(
        hxm->_dma_channel,
        true);

    pio_set_sm_mask_enabled(
        hxm->_pio,
        (1 << hxm->_awaiter_sm) | (1 << hxm->_reader_sm),
        false);

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

#ifndef HX711_NO_MUTEX
    mutex_exit(&hxm->_mut);
#endif

}

void hx711_multi_set_gain(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        assert(hx711_multi__is_state_machines_enabled(hxm));

        const uint32_t gainVal = hx711_gain_to_pio_gain(gain);

        assert(hx711_is_pio_gain_valid(gain));

        pio_sm_drain_tx_fifo(
            hxm->_pio,
            hxm->_reader_sm);

        pio_sm_put(
            hxm->_pio,
            hxm->_reader_sm,
            gainVal);

        hx711_multi_async_start(hxm);

        while(!hx711_multi_async_done(hxm)) {
            tight_loop_contents();
        }

}

void hx711_multi_get_values(
    hx711_multi_t* const hxm,
    int32_t* const values) {

        assert(hx711_multi__is_state_machines_enabled(hxm));
        assert(values != NULL);
        assert(!hx711_multi__async_is_running(hxm));

        hx711_multi_async_start(hxm);
        while(!hx711_multi_async_done(hxm)) {
            tight_loop_contents();
        }
        hx711_multi_async_get_values(hxm, values);

}

bool hx711_multi_get_values_timeout(
    hx711_multi_t* const hxm,
    int32_t* const values,
    const uint timeout) {

        assert(hx711_multi__is_state_machines_enabled(hxm));
        assert(values != NULL);
        assert(!hx711_multi__async_is_running(hxm));

        const absolute_time_t end = make_timeout_time_us(timeout);
        bool success = false;

        hx711_multi_async_start(hxm);

        while(!time_reached(end)) {
            if(hx711_multi_async_done(hxm)) {
                success = true;
                break;
            }
        }

        if(success) {
            hx711_multi_async_get_values(hxm, values);
        }
        else {
            //if timed out, cancel DMA and stop listening
            //for IRQs and exit mutex. Do this atomically!
            UTIL_INTERRUPTS_OFF_BLOCK(
                hx711_multi__async_finish(hxm);
            );
        }

        return success;

}

void hx711_multi_async_start(hx711_multi_t* const hxm) {

    assert(hx711_multi__is_state_machines_enabled(hxm));
    assert(!hx711_multi__async_is_running(hxm));

    //if starting the following statements would lead to an
    //immediate interrupt, DMA may not be properly set up,
    //so disable until it is
    const uint32_t status = save_and_disable_interrupts();

#ifndef HX711_NO_MUTEX
    mutex_enter_blocking(&hxm->_mut);
#endif

    hxm->_async_state = HX711_MULTI_ASYNC_STATE_WAITING;

    //if pio interrupt is already set, we can bypass the
    //IRQ handler and immediately trigger dma
    if(pio_interrupt_get(hxm->_pio, HX711_MULTI_CONVERSION_DONE_IRQ_NUM)) {
        hx711_multi__async_start_dma(hxm);
    }
    else {
        pio_set_irqn_source_enabled(
            hxm->_pio,
            hxm->_pio_irq_index,
            util_pio_get_irq_from_index(hxm->_pio, hxm->_pio_irq_index),
            true);
    }

    restore_interrupts(status);

}

bool hx711_multi_async_done(hx711_multi_t* const hxm) {
    assert(hx711_multi__is_initd(hxm));
    return hxm->_async_state == HX711_MULTI_ASYNC_STATE_DONE;
}

void hx711_multi_async_get_values(
    hx711_multi_t* const hxm,
    int32_t* const values) {
        assert(hx711_multi__is_initd(hxm));
        assert(hx711_multi_async_done(hxm));
        hx711_multi_pinvals_to_values(
            hxm->_buffer,
            values,
            hxm->_chips_len);
}

void hx711_multi_power_up(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain) {

        assert(hx711_multi__is_initd(hxm));

        const uint32_t pioGainVal = hx711_gain_to_pio_gain(gain);

        assert(hx711_is_pio_gain_valid(pioGainVal));

        HX711_MUTEX_BLOCK(hxm->_mut, 

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
                pioGainVal);

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

    HX711_MUTEX_BLOCK(hxm->_mut,

        UTIL_INTERRUPTS_OFF_BLOCK(
            hx711_multi__async_finish(hxm);
        );

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

uint32_t hx711_multi_get_sync_state(
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
        const uint32_t state = hx711_multi_get_sync_state(hxm);

        return state == 0 || state == allReady;

}
