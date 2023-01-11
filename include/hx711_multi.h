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
#include "hx711.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint _HX711_MULTI_CHIPS_READY_IRQ_NUM = 0;
static const uint _HX711_MULTI_READER_READY_IRQ_NUM = 1;
static const uint _HX711_MULTI_MAX_CHIPS = 13;
static uint16_t _HX711_MULTI_TX_FIFO_MUX_BUFFER[_HX711_MULTI_MAX_CHIPS]{0};
static uint32_t _HX711_MULTI_VALUE_BUFFER[_HX711_MULTI_MAX_CHIPS]{0};

typedef struct {

    uint clock_pin;
    uint data_pin_base;

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
    hx711_multi_collection_t* const coll,
    const size_t chipsLen,
    PIO pio);

void hx711_multi_close(hx711_multi_collection_t* const coll) {
    pio_remove_program(coll->_waiter_prog);
    pio_remove_program(coll->_reader_prog);
}

void hx711_multi_sync(hx711_multi_collection_t* const coll);

void hx711_multi_read(
    hx711_multi_collection_t* const coll,
    int32_t* values) {

        uint32_t pinBits;
        uint bit;

        //wait for chips to become ready
        while(!pio_interrupt_get(coll->_pio, _HX711_MULTI_CHIPS_READY_IRQ_NUM));

        //read 24 times
        for(uint i = 0; i < 24; ++i) {
            //read 13 bits of pin values
            //each bit is one pin's value
            pinBits = pio_sm_get_blocking(coll->_pio, coll->_reader_sm);
            //iterate over the 13 bits
            for(uint j = 0; i < _HX711_MULTI_MAX_CHIPS; ++j) {
                //iterate over all chips
                //set the i-th bit of the j-th chip
                bit = (pinBits >> j) & 1;
                _HX711_MULTI_VALUE_BUFFER[j] = _HX711_MULTI_VALUE_BUFFER[j] & ~(1 << i) | (bit << j);
            }
        }

        pio_interrupt_clear(coll->_pio, _HX711_MULTI_READER_READY_IRQ_NUM);

        //now convert all the raw vals to real vals
        for(uint i = 0; i < coll->_chips_len; ++i) {
            values[i] = hx711_get_twos_comp(_HX711_MULTI_VALUE_BUFFER[i]);
        }

}

#ifdef __cplusplus
}
#endif

#endif
