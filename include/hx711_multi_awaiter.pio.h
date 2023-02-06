// THIS FILE IS AUTOGENERATED BY GITHUB ACTIONS
// YOU SHOULD #include THIS FILE
// THE src/hx711_multi_awaiter.pio file IS NOT USABLE UNLESS
// YOU CONVERT IT USING pioasm
// SEE: /.github/workflows/generate_pio.yml

// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ------------------- //
// hx711_multi_awaiter //
// ------------------- //

#define hx711_multi_awaiter_wrap_target 0
#define hx711_multi_awaiter_wrap 6

#define hx711_multi_awaiter_offset_wait_in_pins_bit_count 0u

static const uint16_t hx711_multi_awaiter_program_instructions[] = {
            //     .wrap_target
    0x4001, //  0: in     pins, 1                    
    0xa046, //  1: mov    y, isr                     
    0x8000, //  2: push   noblock                    
    0x0066, //  3: jmp    !y, 6                      
    0xc044, //  4: irq    clear 4                    
    0x0000, //  5: jmp    0                          
    0xc004, //  6: irq    nowait 4                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program hx711_multi_awaiter_program = {
    .instructions = hx711_multi_awaiter_program_instructions,
    .length = 7,
    .origin = -1,
};

static inline pio_sm_config hx711_multi_awaiter_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + hx711_multi_awaiter_wrap_target, offset + hx711_multi_awaiter_wrap);
    return c;
}

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
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hx711_multi.h"
#include "util.h"
void hx711_multi_awaiter_program_init(hx711_multi_t* const hxm) {
    UTIL_ASSERT_NOT_NULL(hxm)
    UTIL_ASSERT_NOT_NULL(hxm->_pio)
    assert(hxm->_chips_len > 0);
    pio_sm_config cfg = hx711_multi_awaiter_program_get_default_config(
        hxm->_awaiter_offset);
    //replace placeholder in instruction with the number of pins
    //to read
    hxm->_pio->instr_mem[hxm->_awaiter_offset + hx711_multi_awaiter_offset_wait_in_pins_bit_count] = 
        pio_encode_in(pio_pins, hxm->_chips_len);
    //data pins
    pio_sm_set_in_pins(
        hxm->_pio,
        hxm->_awaiter_sm,
        hxm->_data_pin_base);
    pio_sm_set_consecutive_pindirs(
        hxm->_pio,
        hxm->_awaiter_sm,
        hxm->_data_pin_base,
        hxm->_chips_len,
        false);         //false = output pins
    sm_config_set_in_pins(
        &cfg,
        hxm->_data_pin_base);
    //even though the program reads data into the ISR,
    //it does not push any data, so make sure autopushing
    //is disabled
    sm_config_set_in_shift(
        &cfg,
        false,              //false = shift left
        true,               //false = autopush disabled
        hxm->_chips_len);   //autopush threshold
    hxm->_awaiter_default_config = cfg;
}

#endif
