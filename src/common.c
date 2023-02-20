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
#include <stddef.h>
#include "hardware/pio.h"
#include "../include/common.h"
#include "../include/hx711.h"
#include "../include/hx711_reader.pio.h"
#include "../include/hx711_multi.h"
#include "../include/hx711_multi_awaiter.pio.h"
#include "../include/hx711_multi_reader.pio.h"

const hx711_config_t HX711__DEFAULT_CONFIG = {
    .clock_pin = 0,
    .data_pin = 0,
    .pio = pio0,
    .pio_init = hx711_reader_pio_init,
    .reader_prog = &hx711_reader_program,
    .reader_prog_init = hx711_reader_program_init
};

const hx711_multi_config_t HX711__MULTI_DEFAULT_CONFIG = {
    .clock_pin = 0,
    .data_pin_base = 0,
    .chips_len = 0,
    .pio_irq_index = HX711_MULTI_ASYNC_PIO_IRQ_IDX,
    .dma_irq_index = HX711_MULTI_ASYNC_DMA_IRQ_IDX,
    .pio = pio0,
    .pio_init = hx711_multi_pio_init,
    .awaiter_prog = &hx711_multi_awaiter_program,
    .awaiter_prog_init = hx711_multi_awaiter_program_init,
    .reader_prog = &hx711_multi_reader_program,
    .reader_prog_init = hx711_multi_reader_program_init
};

void hx711_get_default_config(hx711_config_t* const cfg) {
    assert(cfg != NULL);
    *cfg = HX711__DEFAULT_CONFIG;
}

void hx711_multi_get_default_config(hx711_multi_config_t* const cfg) {
    assert(cfg != NULL);
    *cfg = HX711__MULTI_DEFAULT_CONFIG;
}
