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
#include <stdint.h>
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "pico/platform.h"
#include "pico/mutex.h"
#include "pico/time.h"
#include "../include/hx711.h"
#include "../include/util.h"

const unsigned short HX711_SETTLING_TIMES[] = {
    400,
    50
};

const unsigned char HX711_SAMPLE_RATES[] = {
    10,
    80
};

const unsigned char HX711_CLOCK_PULSES[] = {
    25,
    26,
    27
};

void hx711_init(
    hx711_t* const hx, 
    const hx711_config_t* const config) {

        assert(!hx711__is_initd(hx));

        assert(hx != NULL);
        assert(config != NULL);

        assert(config->pio != NULL);
        assert(config->pio_init != NULL);

        assert(config->reader_prog != NULL);
        assert(config->reader_prog_init != NULL);

        check_gpio_param(config->clock_pin);
        check_gpio_param(config->data_pin);
        assert(config->clock_pin != config->data_pin);

#ifndef HX711_NO_MUTEX
        mutex_init(&hx->_mut);
#endif

        HX711_MUTEX_BLOCK(hx->_mut, 

            hx->_clock_pin = config->clock_pin;
            hx->_data_pin = config->data_pin;
            hx->_pio = config->pio;
            hx->_reader_prog = config->reader_prog;

            util_gpio_set_output(hx->_clock_pin);

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

            gpio_set_input_enabled(
                hx->_data_pin,
                true);

            /**
             * There was originally a call here to gpio_pull_up
             * on the data pin to prevent erroneous data ready
             * states. This was incorrect. Page 4 of the datasheet
             * states: "The 25th pulse at PD_SCK input will pull
             * DOUT pin back to high (Fig.2)."
             */

            //either statement below will panic if it fails
            hx->_reader_offset = pio_add_program(
                hx->_pio,
                hx->_reader_prog);

            hx->_reader_sm = (uint)pio_claim_unused_sm(
                hx->_pio,
                true);

            config->pio_init(hx);
            config->reader_prog_init(hx);

        );

}

void hx711_close(hx711_t* const hx) {

    //state machines do not have to be running in order
    //to close
    assert(hx711__is_initd(hx));

    HX711_MUTEX_BLOCK(hx->_mut, 

        pio_sm_set_enabled(
            hx->_pio,
            hx->_reader_sm,
            false);

        pio_sm_unclaim(
            hx->_pio,
            hx->_reader_sm);

        pio_remove_program(
            hx->_pio,
            hx->_reader_prog,
            hx->_reader_offset);

    );

}

void hx711_set_gain(hx711_t* const hx, const hx711_gain_t gain) {

    assert(hx711__is_state_machine_enabled(hx));
    assert(hx711_is_gain_valid(gain));

    const uint32_t pioGain = hx711_gain_to_pio_gain(gain);

    assert(hx711_is_pio_gain_valid(pioGain));

    HX711_MUTEX_BLOCK(hx->_mut, 

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
            hx->_reader_sm);

        pio_sm_put(
            hx->_pio,
            hx->_reader_sm,
            pioGain);

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
            hx->_reader_sm);

        //2. wait until the value from the currently-set gain
        //can be safely read and discarded
        pio_sm_get_blocking(
            hx->_pio,
            hx->_reader_sm);

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

    );

}

int32_t hx711_get_twos_comp(const uint32_t raw) {
    return
        (int32_t)(-(raw & +HX711_MIN_VALUE)) + 
        (int32_t)(raw & HX711_MAX_VALUE);
}

bool hx711_is_min_saturated(const int32_t val) {
    assert(hx711_is_value_valid(val));
    return val == HX711_MIN_VALUE;
}

bool hx711_is_max_saturated(const int32_t val) {
    assert(hx711_is_value_valid(val));
    return val == HX711_MAX_VALUE;
}

unsigned short hx711_get_settling_time(const hx711_rate_t rate) {
    assert(hx711_is_rate_valid(rate));
    assert((int)rate <= count_of(HX711_SETTLING_TIMES) - 1);
    return HX711_SETTLING_TIMES[(int)rate];
}

unsigned char hx711_get_rate_sps(const hx711_rate_t rate) {
    assert(hx711_is_rate_valid(rate));
    assert((int)rate <= count_of(HX711_SAMPLE_RATES) - 1);
    return HX711_SAMPLE_RATES[(int)rate];
}

unsigned char hx711_get_clock_pulses(const hx711_gain_t gain) {
    assert(hx711_is_gain_valid(gain));
    assert((int)gain <= count_of(HX711_CLOCK_PULSES) - 1);
    return HX711_CLOCK_PULSES[(int)gain];
}

int32_t hx711_get_value(hx711_t* const hx) {

    assert(hx711__is_state_machine_enabled(hx));

    uint32_t rawVal;

    HX711_MUTEX_BLOCK(hx->_mut, 

        /**
         * Block until a value is available
         * 
         * NOTE: remember that reading from the RX FIFO
         * simultaneously clears it. That's why we can keep
         * calling this function hx711_get_value and be
         * assured we'll be getting a new value each time,
         * even if the RX FIFO is currently empty.
         */
        rawVal = pio_sm_get_blocking(
            hx->_pio,
            hx->_reader_sm);

    );

    return hx711_get_twos_comp(rawVal);

}

bool hx711_get_value_timeout(
    hx711_t* const hx,
    int32_t* const val,
    const uint timeout) {

        assert(hx711__is_state_machine_enabled(hx));
        assert(val != NULL);

        bool success = false;
        const absolute_time_t endTime = make_timeout_time_us(timeout);
        uint32_t tempVal;

        assert(!is_nil_time(endTime));

        HX711_MUTEX_BLOCK(hx->_mut, 
            while(!time_reached(endTime)) {
                if((success = hx711__try_get_value(hx->_pio, hx->_reader_sm, &tempVal))) {
                    break;
                }
            }
        );

        if(success) {
            *val = hx711_get_twos_comp(tempVal);
        }

        return success;

}

bool hx711_get_value_noblock(
    hx711_t* const hx,
    int32_t* const val) {

        assert(hx711__is_state_machine_enabled(hx));
        assert(val != NULL);

        bool success;
        uint32_t tempVal;

        HX711_MUTEX_BLOCK(hx->_mut, 
            success = hx711__try_get_value(
                hx->_pio,
                hx->_reader_sm,
                &tempVal);
        );

        if(success) {
            *val = hx711_get_twos_comp(tempVal);
        }

        return success;

}

bool hx711__is_initd(hx711_t* const hx) {
    return hx != NULL &&
        hx->_pio != NULL &&
#ifndef HX711_NO_MUTEX
        mutex_is_initialized(&hx->_mut) &&
#endif
        pio_sm_is_claimed(hx->_pio, hx->_reader_sm);
}

bool hx711__is_state_machine_enabled(hx711_t* const hx) {
    return hx711__is_initd(hx) &&
        util_pio_sm_is_enabled(hx->_pio, hx->_reader_sm);
}

bool hx711_is_value_valid(const int32_t v) {
    return util_int32_t_in_range(
        v,
        HX711_MIN_VALUE,
        HX711_MAX_VALUE);
}

bool hx711_is_pio_gain_valid(const uint32_t g) {
    return util_uint32_t_in_range(
        g,
        HX711_PIO_MIN_GAIN,
        HX711_PIO_MAX_GAIN);
}

bool hx711_is_rate_valid(const hx711_rate_t r) {
    return util_int_in_range(
        (int)r,
        hx711_rate_10,
        hx711_rate_80);
}

bool hx711_is_gain_valid(const hx711_gain_t g) {
    return util_int_in_range(
        (int)g,
        hx711_gain_128,
        hx711_gain_64);
}

void hx711_power_up(
    hx711_t* const hx,
    const hx711_gain_t gain) {

        //the struct should be init'd, but the state machines
        //should not be running
        assert(hx711__is_initd(hx));
        assert(!hx711__is_state_machine_enabled(hx));

        assert(hx711_is_gain_valid(gain));

        const uint32_t gainVal = hx711_gain_to_pio_gain(gain);

        assert(hx711_is_pio_gain_valid(gainVal));

        HX711_MUTEX_BLOCK(hx->_mut, 

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
            gpio_put(
                hx->_clock_pin,
                false);

            //2. reset the state machine using the default config
            //obtained when init'ing.
            pio_sm_init(
                hx->_pio,
                hx->_reader_sm,
                hx->_reader_offset,
                &hx->_reader_prog_default_config);

            //make sure TX FIFO is empty before putting the 
            //gain in.
            pio_sm_drain_tx_fifo(
                hx->_pio,
                hx->_reader_sm);

            //3. Push the initial gain into the TX FIFO
            pio_sm_put(
                hx->_pio,
                hx->_reader_sm,
                gainVal);

            //4. start the state machine
            pio_sm_set_enabled(
                hx->_pio,
                hx->_reader_sm,
                true);

        );

}

void hx711_power_down(hx711_t* const hx) {

    //don't have to have SMs running; just check for init
    assert(hx711__is_initd(hx));

    HX711_MUTEX_BLOCK(hx->_mut, 

        //1. stop the state machine
        pio_sm_set_enabled(
            hx->_pio,
            hx->_reader_sm,
            false);

        /**
         * 2. set clock pin high to start the power down
         * process
         *
         * NOTE: the HX711 chip requires the clock pin to
         * be held high for 60+ us
         * calling functions should therefore do:
         * 
         * hx711_power_down(&hx);
         * hx711_wait_power_down();
         */
        gpio_put(
            hx->_clock_pin,
            true);

    );

}

void hx711_wait_settle(const hx711_rate_t rate) {
    sleep_ms(hx711_get_settling_time(rate));
}

void hx711_wait_power_down() {
    sleep_us(HX711_POWER_DOWN_TIMEOUT);
}

uint32_t hx711_gain_to_pio_gain(const hx711_gain_t gain) {

    /**
     * gain value is 0-based and calculated by:
     * gain = clock pulses - 24 - 1
     * ie. gain of 128 is 25 clock pulses, so
     * gain = 25 - 24 - 1
     * gain = 0
     */

    assert(hx711_is_gain_valid(gain));

    const uint32_t clockPulses =
        hx711_get_clock_pulses(gain) - HX711_READ_BITS - 1;

    assert(hx711_is_pio_gain_valid(clockPulses));

    return clockPulses;

}

bool hx711__try_get_value(
    PIO const pio,
    const uint sm,
    uint32_t* const val) {

        assert(pio != NULL);
        check_sm_param(sm);
        assert(util_pio_sm_is_enabled(pio, sm));
        assert(val != NULL);

        static const uint byteThreshold = HX711_READ_BITS / 8;

        return util_pio_sm_try_get(
            pio,
            sm,
            val,
            byteThreshold);

}
