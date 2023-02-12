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

#include "../include/util.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/platform.h"
#include "pico/time.h"

uint32_t util_dma_get_transfer_count(const uint channel) {
    check_dma_channel_param(channel);
    return (uint32_t)dma_hw->ch[channel].transfer_count;
}

bool util_dma_channel_wait_for_finish_timeout(
    const uint channel,
    const absolute_time_t* end) {

        check_dma_channel_param(channel);
        UTIL_ASSERT_NOT_NULL(end)
        assert(!is_nil_time(*end));

        while(!time_reached(*end)) {
            if(!dma_channel_is_busy(channel)) {
                return true;
            }
        }

        return false;

}

uint util_dma_get_irqn(const uint irq_num) {
    UTIL_ASSERT_RANGE(irq_num, 0, 1)
    return DMA_IRQ_0 + irq_num;
}

void util_dma_channel_set_quiet(
    const uint channel,
    const bool quiet) {
        check_dma_channel_param(channel);
        dma_channel_config cfg = dma_get_channel_config(channel);
        channel_config_set_irq_quiet(&cfg, quiet);
        dma_channel_set_config(channel, &cfg, false);
}

void util_gpio_set_contiguous_input_pins(
    const uint base,
    const uint len) {

        assert(len > 0);

        const uint l = base + len - 1;

        for(uint i = base; i <= l; ++i) {
            check_gpio_param(i);
            gpio_set_input_enabled(i, true);
        }

}

void util_gpio_set_output(const uint gpio) {
    check_gpio_param(gpio);
    gpio_init(gpio);
    gpio_set_dir(gpio, true);
}

uint util_pion_get_irqn(
    const PIO pio,
    const uint irq_num) {

        check_pio_param(pio);
        UTIL_ASSERT_RANGE(irq_num, 0, 1)

        const uint irqn = PIO0_IRQ_0 +
            (pio == pio0 ? 0 : 2) +
            (irq_num == 0 ? 0 : 2);

        UTIL_ASSERT_RANGE(irqn, PIO0_IRQ_0, PIO1_IRQ_1)

        return irqn;

}

uint util_pio_get_pis(
    const uint pio_interrupt_num) {
        UTIL_ASSERT_RANGE(pio_interrupt_num, 0, 3)
        const uint basePis = pis_interrupt0;
        const uint pis = basePis + pio_interrupt_num;
        UTIL_ASSERT_RANGE(pis, pis_interrupt0, pis_interrupt3)
        return pis;
}

void util_pio_gpio_contiguous_init(
    const PIO pio,
    const uint base,
    const uint len) {

        check_pio_param(pio);
        assert(len > 0);

        const uint l = base + len - 1;

        for(uint i = base; i <= l; ++i) {
            check_gpio_param(i);
            pio_gpio_init(pio, i);
        }

}

void util_pio_sm_clear_rx_fifo(
    const PIO pio,
    const uint sm) {
        check_pio_param(pio);
        check_sm_param(sm);
        while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
            pio_sm_get(pio, sm);
        }
}

void util_pio_sm_clear_osr(
    const PIO pio,
    const uint sm) {
        check_pio_param(pio);
        check_sm_param(sm);
        const uint instr = pio_encode_push(false, false);
        while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
            pio_sm_exec(pio, sm, instr);
        }
}

bool util_pio_sm_is_enabled(
    const PIO pio,
    const uint sm) {
        check_pio_param(pio);
        check_sm_param(sm);
        return (pio->ctrl & (1u << (PIO_CTRL_SM_ENABLE_LSB + sm))) != 0;
}

void util_pio_interrupt_wait(
    const PIO pio,
    const uint pio_interrupt_num) {
        check_pio_param(pio);
        while(!pio_interrupt_get(pio, pio_interrupt_num)) {
            tight_loop_contents();
        }
}

void util_pio_interrupt_wait_cleared(
    const PIO pio,
    const uint pio_interrupt_num) {
        check_pio_param(pio);
        while(pio_interrupt_get(pio, pio_interrupt_num)) {
            tight_loop_contents();
        }
}

bool util_pio_interrupt_wait_cleared_timeout(
    const PIO pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end) {

        check_pio_param(pio);
        UTIL_ASSERT_NOT_NULL(end)
        assert(!is_nil_time(*end));

        while(!time_reached(*end)) {
            if(!pio_interrupt_get(pio, pio_interrupt_num)) {
                return true;
            }
        }

        return false;

}

void util_pio_interrupt_wait_clear(
    const PIO pio,
    const uint pio_interrupt_num) {
        check_pio_param(pio);
        util_pio_interrupt_wait(pio, pio_interrupt_num);
        pio_interrupt_clear(pio, pio_interrupt_num);
}

bool util_pio_interrupt_wait_timeout(
    const PIO pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end) {

        check_pio_param(pio);
        assert(end != NULL);
        assert(!is_nil_time(*end));

        while(!time_reached(*end)) {
            if(pio_interrupt_get(pio, pio_interrupt_num)) {
                return true;
            }
        }

        return false;

}

bool util_pio_interrupt_wait_clear_timeout(
    const PIO pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end) {

        check_pio_param(pio);
        UTIL_ASSERT_NOT_NULL(end)
        assert(!is_nil_time(*end));

        const bool ok = util_pio_interrupt_wait_timeout(
            pio,
            pio_interrupt_num,
            end);

        if(ok) {
            pio_interrupt_clear(
                pio,
                pio_interrupt_num);
        }

        return ok;

}

bool util_pio_sm_try_get(
    const PIO pio,
    const uint sm,
    uint32_t* const word,
    const uint threshold) {

        check_pio_param(pio);
        check_sm_param(sm);
        UTIL_ASSERT_NOT_NULL(word)

        if(pio_sm_get_rx_fifo_level(pio, sm) >= threshold) {
            *word = pio_sm_get(pio, sm);
            return true;
        }

        return false;

}
