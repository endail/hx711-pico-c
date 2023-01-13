// THIS FILE IS AUTOGENERATED BY GITHUB ACTIONS
// YOU SHOULD #include THIS FILE
// THE src/hx711_noblock_multi_waiter.pio file IS NOT USABLE UNLESS
// YOU CONVERT IT USING pioasm
// SEE: /.github/workflows/generate_pio.yml

// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// -------------------------- //
// hx711_noblock_multi_waiter //
// -------------------------- //

#define hx711_noblock_multi_waiter_wrap_target 0
#define hx711_noblock_multi_waiter_wrap 16

#define hx711_noblock_multi_waiter_offset_wait_in_pins_bit_count 1u
#define hx711_noblock_multi_waiter_offset_bitloop_in_pins_bit_count 7u

static const uint16_t hx711_noblock_multi_waiter_program_instructions[] = {
            //     .wrap_target
    0xc030, //  0: irq    wait 0 rel                 
    0x4001, //  1: in     pins, 1                    
    0xa0c2, //  2: mov    isr, y                     
    0x0065, //  3: jmp    !y, 5                      
    0x0001, //  4: jmp    1                          
    0xe058, //  5: set    y, 24                      
    0xe001, //  6: set    pins, 1                    
    0x4001, //  7: in     pins, 1                    
    0xe000, //  8: set    pins, 0                    
    0x8020, //  9: push   block                      
    0x0086, // 10: jmp    y--, 6                     
    0x8080, // 11: pull   noblock                    
    0x6022, // 12: out    x, 2                       
    0x1020, // 13: jmp    !x, 0                  [16]
    0xa041, // 14: mov    y, x                       
    0xe001, // 15: set    pins, 1                    
    0x108f, // 16: jmp    y--, 15                [16]
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program hx711_noblock_multi_waiter_program = {
    .instructions = hx711_noblock_multi_waiter_program_instructions,
    .length = 17,
    .origin = -1,
};

static inline pio_sm_config hx711_noblock_multi_waiter_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + hx711_noblock_multi_waiter_wrap_target, offset + hx711_noblock_multi_waiter_wrap);
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
void hx711_noblock_multi_waiter_program_init(hx711_multi_t* const hxm) {
}

#endif