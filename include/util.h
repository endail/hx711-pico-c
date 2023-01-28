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

#ifndef UTIL_H_BC9FF78B_B978_444A_8AA1_FF169B09B09E
#define UTIL_H_BC9FF78B_B978_444A_8AA1_FF169B09B09E

#include <stdint.h>
#include "hardware/dma.h"
#include "hardware/pio.h"

#ifdef __cplusplus
extern "C" {
#endif

void util_pio_sm_clear_rx_fifo(const PIO pio, const uint sm) {
    const uint instr = pio_encode_push(false, false);
    while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
        pio_sm_exec(pio, sm, instr);
    }
}

uint32_t util_dma_get_transfer_count_remaining(const uint channel) {
    check_dma_channel_param(channel);
    return (uint32_t)dma_hw->ch[channel].transfer_count;
}

#ifdef __cplusplus
}
#endif

#endif
