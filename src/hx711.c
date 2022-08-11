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

#include "../include/hx711.h"
#include "hardware/gpio.h"

void hx711_init(
    hx711_t* const hx, 
    const uint clk,
    const uint dat,
    PIO const pio,
    const pio_program_t* const prog,
    hx711_program_init_t prog_init_func) {

        assert(hx != NULL);
        assert(pio != NULL);
        assert(prog != NULL);
        assert(prog_init_func != NULL);
        assert(pio_can_add_program(pio, prog));

        mutex_init(&hx->_mut);
        mutex_enter_blocking(&hx->_mut);

        hx->clock_pin = clk;
        hx->data_pin = dat;
        hx->_pio = pio;
        hx->_prog = prog;

        gpio_init(hx->clock_pin);
        gpio_set_dir(hx->clock_pin, GPIO_OUT);

        /**
         * There was originally a call here to gpio_put on the
         * clock pin to power up the HX711. I have decided to
         * remove this and also remove enabling the state
         * machine from the pio init function. This does leave
         * the HX711's power state undefined from the
         * perspective of the code, but does give a much clearer
         * separation of duties. This function merely init's the
         * hardware and state machine, and the hx711_set_power
         * function sets the power and enables/disables the
         * state machine.
         */

        gpio_init(hx->data_pin);
        gpio_set_dir(hx->data_pin, GPIO_IN);

        /**
         * There was originally a call here to gpio_pull_up
         * on the data pin to prevent erroneous data ready
         * states. This was incorrect. Page 4 of the datasheet
         * states: "The 25th pulse at PD_SCK input will pull
         * DOUT pin back to high (Fig.2)."
         */

        //either statement below will panic if it fails
        hx->_offset = pio_add_program(hx->_pio, hx->_prog);
        hx->_state_mach = (uint)pio_claim_unused_sm(hx->_pio, true);

        prog_init_func(hx);

        mutex_exit(&hx->_mut);

}

void hx711_close(hx711_t* const hx) {

    assert(hx != NULL);
    assert(hx->_pio != NULL);
    assert(pio_sm_is_claimed(hx->_pio, hx->_state_mach));
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

    mutex_exit(&hx->_mut);

}

void hx711_set_gain(hx711_t* const hx, const hx711_gain_t gain) {

    assert(hx != NULL);
    assert(hx->_pio != NULL);
    assert(pio_sm_is_claimed(hx->_pio, hx->_state_mach));
    assert(mutex_is_initialized(&hx->_mut));

    /**
     * gain value is 0-based and calculated by:
     * gain = clock pulses - 24 - 1
     * ie. gain of 128 is 25 clock pulses, so
     * gain = 25 - 24 - 1
     * gain = 0
     */
    const uint32_t gainVal = (uint32_t)gain - HX711_READ_BITS - 1;

    assert(gainVal <= 2);

    mutex_enter_blocking(&hx->_mut);

    /**
     * Before putting anything in the TX FIFO buffer,
     * assume the worst-case scenario which is that
     * there's something already in there. There ought
     * not to be, but clearing it ensures the following
     * pio_sm_put* call does not need to block as this
     * function to change the gain should take precedence.
     */
    pio_sm_drain_tx_fifo(
        hx->_pio,
        hx->_state_mach);

    pio_sm_put(
        hx->_pio,
        hx->_state_mach,
        gainVal);

    /**
     * At this point the current value in the RX FIFO will
     * have been calculated based on whatever the previous
     * set gain was. So, the RX FIFO needs to be cleared.
     * 
     * NOTE: checking for whether the RX FIFO is not empty
     * won't work. A conversion may have already begun
     * before any bits have been moved into the ISR.
     * 
     * UPDATE: the worst-case scenario here is that the
     * pio_sm_put call has occurred after the pio "pull",
     * because we then need to wait until the following
     * "pull" in the state machine. If this happens:
     * 
     * 1. there may already be a value in the RX FIFO; and
     * 2. another value will need to be read and discarded
     * following which the new gain will be set.
     * 
     * To handle 1.: Clear the RX FIFO with a non-blocking
     * read. If the RX FIFO is empty, no harm done because
     * the call won't block.
     * 
     * To handle 2.: Read the "next" value with a blocking
     * read to ensure the "next, next" value will be set
     * to the desired gain.
     */

    //1. clear the RX FIFO with the non-blocking read
    pio_sm_get(
        hx->_pio,
        hx->_state_mach);

    //2. wait until the value from the currently-set gain
    //can be safely read and discarded
    pio_sm_get_blocking(
        hx->_pio,
        hx->_state_mach);

    /**
     * Immediately following the above blocking call, the
     * state machine will pull in the data in the
     * pio_sm_put call above and pulse the HX711 the
     * correct number of times to set the desired gain.
     * 
     * No further communication with the state machine
     * from this function is required. Any other function(s)
     * wishing to obtain a value from the HX711 need only
     * block until one is there (or check the RX FIFO level).
     */

    mutex_exit(&hx->_mut);

}

int32_t hx711_get_value(hx711_t* const hx) {

    assert(hx != NULL);
    assert(hx->_pio != NULL);
    assert(pio_sm_is_claimed(hx->_pio, hx->_state_mach));
    assert(mutex_is_initialized(&hx->_mut));

    mutex_enter_blocking(&hx->_mut);

    /**
     * Block until a value is available
     * 
     * NOTE: remember that reading from the RX FIFO
     * simultaneously clears it. That's why we can keep
     * calling this function hx711_get_value and be
     * assured we'll be getting a new value each time,
     * even if the RX FIFO is currently empty.
     */
    const uint32_t rawVal = pio_sm_get_blocking(
        hx->_pio,
        hx->_state_mach);

    mutex_exit(&hx->_mut);

    return hx711_get_twos_comp(rawVal);

}

bool hx711_get_value_timeout(
    hx711_t* const hx,
    const uint64_t* const timeout,
    int32_t* const val) {

        assert(hx != NULL);
        assert(hx->_pio != NULL);
        assert(val != NULL);
        assert(timeout != NULL);
        assert(pio_sm_is_claimed(hx->_pio, hx->_state_mach));
        assert(mutex_is_initialized(&hx->_mut));

        bool success = false;
        static const uint byteThreshold = HX711_READ_BITS / 8;
        uint32_t tempVal;
        const absolute_time_t endTime = make_timeout_time_us(*timeout);

        assert(!is_nil_time(endTime));

        mutex_enter_blocking(&hx->_mut);

        while(!time_reached(endTime)) {
            if(pio_sm_get_rx_fifo_level(hx->_pio, hx->_state_mach) >= byteThreshold) {
                //obtain value and relinquish the mutex ASAP
                tempVal = pio_sm_get(hx->_pio, hx->_state_mach);
                success = true;
                break;
            }
        }

        mutex_exit(&hx->_mut);

        if(success) {
            *val = hx711_get_twos_comp(tempVal);
        }

        return success;

}

void hx711_set_power(hx711_t* const hx, const hx711_power_t pwr) {

    assert(hx != NULL);
    assert(hx->_pio != NULL);
    assert(pio_sm_is_claimed(hx->_pio, hx->_state_mach));
    assert(mutex_is_initialized(&hx->_mut));

    mutex_enter_blocking(&hx->_mut);

    if(pwr == hx711_pwr_up) {

        /**
         * NOTE: pio_sm_restart should not be used here.
         * That function clears the clock divider which is
         * currently set in the dedicated pio init function.
         */

        /**
         * 1. set the clock pin low to power up the chip
         * 
         * There does not appear to be any delay after
         * powering up. Any actual delay would presumably be
         * dealt with by the HX711 prior to the data pin
         * going low. Which, in turn, is handled by the state
         * machine in waiting for the low signal.
         */
        gpio_put(hx->clock_pin, false);

        //2. reset the state machine using the default config
        //obtained when init'ing.
        pio_sm_init(
            hx->_pio,
            hx->_state_mach,
            hx->_offset,
            &hx->_default_config);

        //3. start the state machine
        pio_sm_set_enabled(
            hx->_pio,
            hx->_state_mach,
            true);

    }
    else if(pwr == hx711_pwr_down) {

        //1. stop the state machine
        pio_sm_set_enabled(
            hx->_pio,
            hx->_state_mach,
            false);

        /**
         * 2. set clock pin high to start the power down
         * process
         *
         * NOTE: the HX711 chip requires the clock pin to
         * be held high for 60+ us
         * calling functions should therefore do:
         * 
         * hx711_set_power(hx, hx711_pwr_down);
         * hx711_wait_power_down();
         */
        gpio_put(hx->clock_pin, true);

    }

    mutex_exit(&hx->_mut);

}
