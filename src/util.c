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
#include <stdint.h>
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/pio_instructions.h"
#include "hardware/regs/intctrl.h"
#include "hardware/regs/pio.h"
#include "hardware/structs/dma.h"
#include "hardware/timer.h"
#include "pico/platform.h"
#include "pico/time.h"
#include "pico/types.h"
#include "../include/util.h"

#define UTIL_DEF_IN_RANGE_FUNC(TYPE) \
    bool util_ ## TYPE ##_in_range( \
        const TYPE val, \
        const TYPE min, \
        const TYPE max) { \
            return val >= min && val <= max; \
}

const uint8_t util_pio_to_irq_map[] = {
    PIO0_IRQ_0,
    PIO0_IRQ_1,
    PIO1_IRQ_0,
    PIO1_IRQ_1
};

const uint8_t util_dma_to_irq_map[] = {
    DMA_IRQ_0,
    DMA_IRQ_1
};

UTIL_DEF_IN_RANGE_FUNC(int32_t)
UTIL_DEF_IN_RANGE_FUNC(uint32_t)
UTIL_DEF_IN_RANGE_FUNC(int)
UTIL_DEF_IN_RANGE_FUNC(uint)

bool util_dma_irq_index_is_valid(const uint idx) {
    return util_uint_in_range(
        idx,
        UTIL_DMA_IRQ_INDEX_MIN,
        UTIL_DMA_IRQ_INDEX_MAX);
}

uint util_dma_get_irq_from_index(const uint idx) {
    assert(util_dma_irq_index_is_valid(idx));
    assert(util_dma_to_irq_map != NULL);
    return util_dma_to_irq_map[idx];
}

int util_dma_get_index_from_irq(const uint irq_num) {

    check_irq_param(irq_num);
    assert(util_dma_to_irq_map != NULL);

    switch(irq_num) {
        case DMA_IRQ_0:
            return 0;
        case DMA_IRQ_1:
            return 1;
        default:
            return -1;
    }

}

void util_dma_set_exclusive_channel_irq_handler(
    const uint irq_index,
    const uint channel,
    const irq_handler_t handler,
    const bool enabled) {

        assert(util_dma_irq_index_is_valid(irq_index));
        check_dma_channel_param(channel);
        assert(handler != NULL);

        const uint irq_num = util_dma_get_irqn(irq_index);

        dma_irqn_set_channel_enabled(
            irq_index,
            channel,
            enabled);

        irq_set_exclusive_handler(
            irq_num,
            handler);

        irq_set_enabled(
            irq_num,
            enabled);

}

uint32_t util_dma_get_transfer_count(const uint channel) {
    check_dma_channel_param(channel);
    return (uint32_t)dma_hw->ch[channel].transfer_count;
}

bool util_dma_channel_wait_for_finish_timeout(
    const uint channel,
    const absolute_time_t* const end) {

        check_dma_channel_param(channel);
        assert(end != NULL);
        assert(!is_nil_time(*end));

        while(!time_reached(*end)) {
            if(!dma_channel_is_busy(channel)) {
                return true;
            }
        }

        return false;

}

uint util_dma_get_irqn(const uint irq_index) {

    assert(util_dma_to_irq_map != NULL);
    assert(util_uint_in_range(
        irq_index,
        UTIL_DMA_IRQ_INDEX_MIN,
        UTIL_DMA_IRQ_INDEX_MAX));

    const uint irq_num = util_dma_to_irq_map[
        irq_index];

    check_irq_param(irq_num);

    return irq_num;

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

void util_irq_set_exclusive_pio_interrupt_num_handler(
    PIO const pio,
    const uint irq_index,
    const uint pio_interrupt_num,
    const irq_handler_t handler,
    const bool enabled) {

        check_pio_param(pio);
        assert(util_pio_irq_index_is_valid(irq_index));
        assert(util_routable_pio_interrupt_num_is_valid(pio_interrupt_num));
        assert(handler != NULL);

        const uint irq_num = util_pio_get_irq_from_index(
            pio,
            irq_index);

        const uint pis = util_pio_get_pis_from_pio_interrupt_num(
            pio_interrupt_num);

        pio_set_irqn_source_enabled(
            pio,
            irq_index,
            pis,
            enabled);

        irq_set_exclusive_handler(
            irq_num,
            handler);

        irq_set_enabled(
            irq_num,
            enabled);

}

bool util_pio_irq_index_is_valid(const uint idx) {
    return util_uint_in_range(
        idx,
        UTIL_PIO_IRQ_INDEX_MIN,
        UTIL_PIO_IRQ_INDEX_MAX);
}

uint util_pion_get_irqn(
    PIO const pio,
    const uint irq_index) {

        check_pio_param(pio);
        assert(util_pio_irq_index_is_valid(irq_index));
        assert(util_pio_to_irq_map != NULL);

        const uint irq_num = util_pio_to_irq_map[
            pio_get_index(pio) + irq_index];

        check_irq_param(irq_num);

        return irq_num;

}

uint util_pio_get_irq_from_index(
    PIO const pio,
    const uint idx) {
    
        check_pio_param(pio);
        assert(util_pio_irq_index_is_valid(idx));
        assert(util_pio_to_irq_map != NULL);

        const uint irq_num = util_pio_to_irq_map[
            pio_get_index(pio) + idx];

        check_irq_param(irq_num);

        return irq_num;

}

int util_pio_get_index_from_irq(const uint irq_num) {

    check_irq_param(irq_num);

    switch(irq_num) {
        case PIO0_IRQ_0:
        case PIO1_IRQ_0:
            return 0;
        case PIO0_IRQ_1:
        case PIO1_IRQ_1:
            return 1;
        default:
            return -1;
    }

}

PIO const util_pio_get_pio_from_irq(const uint irq_num) {

    check_irq_param(irq_num);

    switch(irq_num) {
        case PIO0_IRQ_0:
        case PIO0_IRQ_1:
            return pio0;
        case PIO1_IRQ_0:
        case PIO1_IRQ_1:
            return pio1;
        default:
            return NULL;
    }

}

uint util_pio_get_pis_from_pio_interrupt_num(
    const uint pio_interrupt_num) {

        assert(util_routable_pio_interrupt_num_is_valid(
            pio_interrupt_num));

        const uint basePis = pis_interrupt0;
        const uint pis = basePis + pio_interrupt_num;

        assert(util_uint_in_range(
            pis, pis_interrupt0, pis_interrupt3));

        return pis;

}

void util_pio_gpio_contiguous_init(
    PIO const pio,
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
    PIO const pio,
    const uint sm) {
        check_pio_param(pio);
        check_sm_param(sm);
        while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
            pio_sm_get(pio, sm);
        }
}

void util_pio_sm_clear_osr(
    PIO const pio,
    const uint sm) {
        check_pio_param(pio);
        check_sm_param(sm);
        const uint instr = pio_encode_push(false, false);
        while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
            pio_sm_exec(pio, sm, instr);
        }
}

void util_pio_sm_clear_isr(
    PIO const pio,
    const uint sm) {
        check_pio_param(pio);
        check_sm_param(sm);
        const uint instr = pio_encode_pull(false, false);
        while(!pio_sm_is_tx_fifo_empty(pio, sm)) {
            pio_sm_exec(pio, sm, instr);
        }
}

bool util_pio_sm_is_enabled(
    PIO const pio,
    const uint sm) {
        check_pio_param(pio);
        check_sm_param(sm);
        return (pio->ctrl & (1u << (PIO_CTRL_SM_ENABLE_LSB + sm))) != 0;
}

bool util_pio_interrupt_num_is_valid(
    const uint pio_interrupt_num) {
        return util_uint_in_range(
            pio_interrupt_num,
            UTIL_PIO_INTERRUPT_NUM_MIN,
            UTIL_PIO_INTERRUPT_NUM_MAX);
}

bool util_routable_pio_interrupt_num_is_valid(
    const uint pio_interrupt_num) {
        return util_uint_in_range(
            pio_interrupt_num,
            UTIL_ROUTABLE_PIO_INTERRUPT_NUM_MIN,
            UTIL_ROUTABLE_PIO_INTERRUPT_NUM_MAX);
}

void util_pio_interrupt_wait(
    PIO const pio,
    const uint pio_interrupt_num) {
        check_pio_param(pio);
        assert(util_pio_interrupt_num_is_valid(pio_interrupt_num));
        while(!pio_interrupt_get(pio, pio_interrupt_num)) {
            tight_loop_contents();
        }
}

void util_pio_interrupt_wait_cleared(
    PIO const pio,
    const uint pio_interrupt_num) {
        check_pio_param(pio);
        assert(util_pio_interrupt_num_is_valid(pio_interrupt_num));
        while(pio_interrupt_get(pio, pio_interrupt_num)) {
            tight_loop_contents();
        }
}

bool util_pio_interrupt_wait_cleared_timeout(
    PIO const pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end) {

        check_pio_param(pio);
        assert(util_pio_interrupt_num_is_valid(pio_interrupt_num));
        assert(end != NULL);
        assert(!is_nil_time(*end));

        while(!time_reached(*end)) {
            if(!pio_interrupt_get(pio, pio_interrupt_num)) {
                return true;
            }
        }

        return false;

}

void util_pio_interrupt_wait_clear(
    PIO const pio,
    const uint pio_interrupt_num) {
        check_pio_param(pio);
        assert(util_pio_interrupt_num_is_valid(pio_interrupt_num));
        util_pio_interrupt_wait(pio, pio_interrupt_num);
        pio_interrupt_clear(pio, pio_interrupt_num);
}

bool util_pio_interrupt_wait_timeout(
    PIO const pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end) {

        check_pio_param(pio);
        assert(util_pio_interrupt_num_is_valid(pio_interrupt_num));
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
    PIO const pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end) {

        check_pio_param(pio);
        assert(util_pio_interrupt_num_is_valid(pio_interrupt_num));
        assert(end != NULL);
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
    PIO const pio,
    const uint sm,
    uint32_t* const word,
    const uint threshold) {

        check_pio_param(pio);
        check_sm_param(sm);
        assert(word != NULL);

        if(pio_sm_get_rx_fifo_level(pio, sm) >= threshold) {
            *word = pio_sm_get(pio, sm);
            return true;
        }

        return false;

}

#undef UTIL_DEF_IN_RANGE_FUNC
