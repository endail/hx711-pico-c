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
#include <stdint.h>
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
    #define CHECK_HX711_INITD(hx) \
        assert(hx != NULL); \
        assert(hx->_pio != NULL); \
        assert(pio_sm_is_claimed(hx->_pio, hx->_state_mach)); \
        assert(mutex_is_initialized(&hx->_mut));
#else
    #define CHECK_HX711_INITD(hx)
#endif

static const uint _HX711_MULTI_DATAS_LOW_IRQ_NUM = 0;
static const uint _HX711_MULTI_READER_READY_IRQ_NUM = 1;

typedef struct {
    uint data_pin;
} hx711_multi_t;

typedef struct {

    uint clock_pin;

    size_t _chips_len;

    PIO _pio;

    const pio_program_t* _waiter_prog;
    pio_sm_config _waiter_default_config;
    uint _waiter_sm;
    uint _waiter_offset;

    const pio_program_t* _reader_prog;
    pio_sm_config _reader_default_config;
    uint _reader_sm;
    uint _reader_offset;

    mutex_t _mut;

} hx711_multi_collection_t;

void hx711_multi_init(
    const uint clockPin,
    PIO pio);

void hx711_multi_sync(hx711_multi_collection_t* const coll);

void hx711_multi_read(
    hx711_multi_collection_t* const coll,
    int32_t* values) {

        // wait for reader 
        while(!pio_interrupt_get(coll->_pio, _HX711_MULTI_READER_READY_IRQ_NUM));

        for(uint i = 0; i < coll->_chips_len; ++i) {
            values[i] = pio_sm_get_blocking(coll->_pio, coll->_reader_sm);
        }

        pio_interrupt_clear(coll->_pio, _HX711_MULTI_READER_READY_IRQ_NUM);

}

#ifdef __cplusplus
}
#endif

#endif
