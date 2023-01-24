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

#ifndef _HX711_MULTI_H_253BF37A_8356_462B_B8F9_39E09A7193E6
#define _HX711_MULTI_H_253BF37A_8356_462B_B8F9_39E09A7193E6

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "pico/platform.h"
#include "hx711.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
    #define CHECK_HX711_MULTI_INITD(hxm) \
        assert(hxm != NULL); \
        assert(hxm->_pio != NULL); \
        assert(pio_sm_is_claimed(hxm->_pio, hxm->_awaiter_sm)); \
        assert(pio_sm_is_claimed(hxm->_pio, hxm->_reader_sm)); \
        assert(mutex_is_initialized(&hxm->_mut));
#else
    #define CHECK_HX711_MULTI_INITD(hxm)
#endif

#define HX711_MULTI_APP_WAIT_IRQ_NUM 0
#define HX711_MULTI_DATA_READY_IRQ_NUM 1
#define HX711_MULTI_WAIT_DATA_READY_IRQ_NUM 2
#define HX711_MULTI_MAX_CHIPS 32

typedef struct {

    uint _clock_pin;
    uint _data_pin_base;
    size_t _chips_len;

    PIO _pio;

    const pio_program_t* _awaiter_prog;
    pio_sm_config _awaiter_default_config;
    uint _awaiter_sm;
    uint _awaiter_offset;

    const pio_program_t* _reader_prog;
    pio_sm_config _reader_default_config;
    uint _reader_sm;
    uint _reader_offset;

    //static array; unit'd is OK, will be overwritten
    uint32_t _read_buffer[HX711_READ_BITS];

    mutex_t _mut;

} hx711_multi_t;

typedef void (*hx711_multi_pio_init_t)(hx711_multi_t* const);
typedef void (*hx711_multi_program_init_t)(hx711_multi_t* const);

typedef struct {

    uint clock_pin;
    uint data_pin_base;
    size_t chips_len;

    PIO pio;
    hx711_multi_pio_init_t pio_init;

    const pio_program_t* awaiter_prog;
    hx711_multi_program_init_t awaiter_prog_init;

    const pio_program_t* reader_prog;
    hx711_multi_program_init_t reader_prog_init;

} hx711_multi_config_t;

void hx711_multi_init(
    hx711_multi_t* const hxm,
    const hx711_multi_config_t* const config);

void hx711_multi_close(hx711_multi_t* const hxm);

void hx711_multi_set_gain(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain);

/**
 * @brief Fill an array with one value from each chip
 * 
 * @param hxm 
 * @param values 
 */
void hx711_multi_get_values(
    hx711_multi_t* const hxm,
    int32_t* values);

void hx711_multi_power_up(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain);

void hx711_multi_power_down(hx711_multi_t* const hxm);

/**
 * @brief Signal to the reader SM that it's time to start
 * reading in values
 * 
 * @param hxm 
 */
static inline void hx711_multi__wait_app_ready(hx711_multi_t* const hxm) {

    CHECK_HX711_MULTI_INITD(hxm)

    //wait until the SM has returned to waiting for the app code
    //this may be unnecessary, but would help to prevent obtaining
    //values too quickly and corrupting the HX711 conversion period
    while(!pio_interrupt_get(hxm->_pio, HX711_MULTI_APP_WAIT_IRQ_NUM)) {
        tight_loop_contents();
    }

    //then clear that irq to allow it to proceed
    pio_interrupt_clear(hxm->_pio, HX711_MULTI_APP_WAIT_IRQ_NUM);

}

/**
 * @brief Convert an array of pinvals to regular HX711
 * values
 * 
 * @param pinvals 
 * @param rawvals 
 * @param len number of values to convert
 */
static void hx711_multi__pinvals_to_values(
    const uint32_t* const pinvals,
    int32_t* const rawvals,
    const size_t len);

/**
 * @brief Reads pinvals into the internal buffer
 * 
 * @param hxm 
 */
static inline void hx711_multi__get_values_raw(
    hx711_multi_t* const hxm) {

        CHECK_HX711_MULTI_INITD(hxm)

        hx711_multi__wait_app_ready(hxm);

        for(size_t i = 0; i < HX711_READ_BITS; ++i) {
            hxm->_read_buffer[i] = pio_sm_get_blocking(
                hxm->_pio,
                hxm->_reader_sm);
        }

}

#ifdef __cplusplus
}
#endif

#endif
