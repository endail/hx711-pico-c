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

#ifndef UTIL_H_BC9FF78B_B978_444A_8AA1_FF169B09B09E
#define UTIL_H_BC9FF78B_B978_444A_8AA1_FF169B09B09E

#include <assert.h>
#include <stdint.h>
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "pico/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the transfer count for a given DMA channel. When a
 * DMA transfer is active, this count is the number of transfers
 * remaining.
 * 
 * @param channel 
 * @return uint32_t 
 */
static inline uint32_t util_dma_get_transfer_count(const uint channel) {
    check_dma_channel_param(channel);
    return (uint32_t)dma_hw->ch[channel].transfer_count;
}

/**
 * @brief Sets GPIO pins from base to base + len to input.
 * 
 * @param base 
 * @param len 
 */
static inline void util_gpio_set_contiguous_input_pins(const uint base, const uint len) {

    assert(len > 0);

    const uint l = base + len - 1;

    for(uint i = base; i <= l; ++i) {
        check_gpio_param(i);
        gpio_set_input_enabled(i, true);
    }

}

/**
 * @brief Inits and sets GPIO pin to output.
 * 
 * @param gpio 
 */
static inline void util_gpio_set_output(const uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, true);
}

/**
 * @brief Inits GPIO pins for PIO from base to base + len.
 * 
 * @param pio 
 * @param base 
 * @param len 
 */
static inline void util_pio_gpio_contiguous_init(
    const PIO pio,
    const uint base,
    const uint len) {
        const uint l = base + len - 1;
        for(uint i = base; i <= l; ++i) {
            pio_gpio_init(pio, i);
        }
}

/**
 * @brief Clears a given state machine's RX FIFO.
 * 
 * @param pio 
 * @param sm 
 */
static inline void util_pio_sm_clear_rx_fifo(
    const PIO pio,
    const uint sm) {
        const uint instr = pio_encode_push(false, false);
        while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
            pio_sm_exec(pio, sm, instr);
        }
}

/**
 * @brief Waits for a given PIO interrupt to be set.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 */
static inline void util_pio_interrupt_wait(
    const PIO pio,
    const uint pio_interrupt_num) {
        while(!pio_interrupt_get(pio, pio_interrupt_num)) {
            tight_loop_contents();
        }
}

/**
 * @brief Waits for a given PIO interrupt to be set and then
 * clears it.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 */
static inline void util_pio_interrupt_wait_clear(
    const PIO pio,
    const uint pio_interrupt_num) {
        util_pio_interrupt_wait(pio, pio_interrupt_num);
        pio_interrupt_clear(pio, pio_interrupt_num);
}

/**
 * @brief Waits for a given PIO to be set within the timeout
 * period.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 * @param timeout_us microseconds
 * @return true if the interrupt was set within the timeout
 * @return false if the timeout was reached
 */
static inline bool util_pio_interrupt_wait_timeout(
    const PIO pio,
    const uint pio_interrupt_num,
    const uint64_t timeout_us) {

        const absolute_time_t endTime = make_timeout_time_us(timeout_us);
        assert(!is_nil_time(endTime));

        while(!time_reached(endTime)) {
            if(pio_interrupt_get(pio, pio_interrupt_num)) {
                return true;
            }
        }

        return false;

}

/**
 * @brief Waits for a given PIO to be set within the timeout
 * period and then clears it.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 * @param timeout_us microseconds
 * @return true if the interrupt was set within the timeout
 * @return false if the timeout was reached
 */
static inline bool util_pio_interrupt_wait_clear_timeout(
    const PIO pio,
    const uint pio_interrupt_num,
    const uint64_t timeout_us) {

        const bool ok = util_pio_interrupt_wait_timeout(
            pio,
            pio_interrupt_num,
            timeout_us);

        if(ok) {
            pio_interrupt_clear(pio, pio_interrupt_num);
        }

        return ok;

}

/**
 * @brief Attempts to get a word from the state machine's RX FIFO
 * if more than threshold words are in the buffer.
 * 
 * @param pio 
 * @param sm 
 * @param word variable to be set
 * @param threshold word count (1 word = 8 bytes)
 * @return true if word was set with the value from the RX FIFO
 * @return false if the number of words in the RX FIFO is below
 * the threshold
 */
static inline bool util_pio_sm_try_get(
    const PIO pio,
    const uint sm,
    uint32_t* const word,
    const uint threshold) {

        assert(word != NULL);

        if(pio_sm_get_rx_fifo_level(pio, sm) >= threshold) {
            *word = pio_sm_get(pio, sm);
            return true;
        }

        return false;

}

#ifdef __cplusplus
}
#endif

#endif
