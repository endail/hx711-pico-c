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

#include <stdint.h>
#include "hardware/pio.h"
#include "hardware/sync.h"
#include "pico/mutex.h"
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UTIL_DMA_IRQ_INDEX_MIN 0u
#define UTIL_DMA_IRQ_INDEX_MAX 1u

#define UTIL_PIO_IRQ_INDEX_MIN 0u
#define UTIL_PIO_IRQ_INDEX_MAX 1u

#define UTIL_PIO_INTERRUPT_NUM_MIN 0u
#define UTIL_PIO_INTERRUPT_NUM_MAX 7u

#define UTIL_ROUTABLE_PIO_INTERRUPT_NUM_MIN 0u
#define UTIL_ROUTABLE_PIO_INTERRUPT_NUM_MAX 3u

#define UTIL_MUTEX_BLOCK(mut, ...) \
    do { \
        mutex_enter_blocking(&mut); \
        __VA_ARGS__ \
        mutex_exit(&mut); \
    } while(0)

#define UTIL_INTERRUPTS_OFF_BLOCK(...) \
    do { \
        const uint32_t _interrupt_status = save_and_disable_interrupts(); \
        __VA_ARGS__ \
        restore_interrupts(_interrupt_status); \
    } while(0)

#define DECL_IN_RANGE_FUNC(TYPE) \
    bool util_ ## TYPE ##_in_range( \
        const TYPE val, \
        const TYPE min, \
        const TYPE max);

DECL_IN_RANGE_FUNC(int32_t)
DECL_IN_RANGE_FUNC(uint32_t)
DECL_IN_RANGE_FUNC(int)
DECL_IN_RANGE_FUNC(uint)

#undef DECL_IN_RANGE_FUNC

/**
 * @brief Check whether a DMA IRQ index is valid.
 * 
 * @param idx 
 * @return true 
 * @return false 
 */
bool util_dma_irq_index_is_valid(const uint idx);

/**
 * @brief Get the transfer count for a given DMA channel. When a
 * DMA transfer is active, this count is the number of transfers
 * remaining.
 * 
 * @param channel 
 * @return uint32_t 
 */
uint32_t util_dma_get_transfer_count(const uint channel);

/**
 * @brief Wait until channel has completed transferring up
 * to a timeout.
 * 
 * @param channel 
 * @param end 
 * @return true if transfer completed
 * @return false is timeout was reached
 */
bool util_dma_channel_wait_for_finish_timeout(
    const uint channel,
    const absolute_time_t* const end);

/**
 * @brief Gets the NVIC IRQ number based on the DMA IRQ
 * index.
 * 
 * @param irq_num 0 or 1
 * @return uint DMA_IRQ_0 or DMA_IRQ_1
 */
uint util_dma_get_irqn(const uint irq_num);

/**
 * @brief Sets a DMA channel's IRQ quiet mode.
 * 
 * @param channel 
 * @param quiet true for quiet otherwise false
 */
void util_dma_channel_set_quiet(
    const uint channel,
    const bool quiet);

/**
 * @brief Sets GPIO pins from base to base + len to input.
 * 
 * @param base 
 * @param len 
 */
void util_gpio_set_contiguous_input_pins(
    const uint base,
    const uint len);

/**
 * @brief Initialises and sets GPIO pin to output.
 * 
 * @param gpio 
 */
void util_gpio_set_output(const uint gpio);

/**
 * @brief Check whether PIO IRQ index is valid
 * 
 * @param idx 
 * @return true 
 * @return false 
 */
bool util_pio_irq_index_is_valid(const uint idx);

/**
 * @brief Gets the correct NVIC IRQ number for a PIO
 * according to the IRQ number.
 * 
 * @example util_pion_get_irqn(pio1, 1); //returns PIO1_IRQ_1
 * 
 * @param pio 
 * @param irq_num 0 or 1
 * @return uint 
 */
uint util_pion_get_irqn(
    PIO const pio,
    const uint irq_num);

/**
 * @brief Gets the correct PIO interrupt source number according
 * to the raw PIO interrupt number used in a .pio file.
 * 
 * @example util_pio_get_pis(3); //returns pis_interrupt3 (11)
 * 
 * @see pio_interrupt_source
 * @param pio_interrupt_num 
 * @return uint 
 */
uint util_pio_get_pis(
    const uint pio_interrupt_num);

/**
 * @brief Inits GPIO pins for PIO from base to base + len.
 * 
 * @param pio 
 * @param base 
 * @param len 
 */
void util_pio_gpio_contiguous_init(
    PIO const pio,
    const uint base,
    const uint len);

/**
 * @brief Clears a given state machine's RX FIFO.
 * 
 * @param pio 
 * @param sm 
 */
void util_pio_sm_clear_rx_fifo(
    PIO const pio,
    const uint sm);

/**
 * @brief Clear's a given state machine's OSR.
 * 
 * @param pio 
 * @param sm 
 */
void util_pio_sm_clear_osr(
    PIO const pio,
    const uint sm);

/**
 * @brief Check whether a given state machine is enabled.
 * 
 * @param pio 
 * @param sm 
 * @return true 
 * @return false 
 */
bool util_pio_sm_is_enabled(
    PIO const pio,
    const uint sm);

bool util_pio_interrupt_num_is_valid(
    const uint pio_interrupt_num);

bool util_routable_pio_interrupt_num_is_valid(
    const uint pio_interrupt_num);

/**
 * @brief Waits for a given PIO interrupt to be set.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 */
void util_pio_interrupt_wait(
    PIO const pio,
    const uint pio_interrupt_num);

/**
 * @brief Waits until a given PIO interrupt is cleared.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 */
void util_pio_interrupt_wait_cleared(
    PIO const pio,
    const uint pio_interrupt_num);

/**
 * @brief Waits until the given interrupt is cleared, up to
 * a maximum timeout.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 * @param end 
 * @return true 
 * @return false 
 */
bool util_pio_interrupt_wait_cleared_timeout(
    PIO const pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end);

/**
 * @brief Waits for a given PIO interrupt to be set and then
 * clears it.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 */
void util_pio_interrupt_wait_clear(
    PIO const pio,
    const uint pio_interrupt_num);

/**
 * @brief Waits for a given PIO to be set within the timeout
 * period.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 * @param end
 * @return true if the interrupt was set within the timeout
 * @return false if the timeout was reached
 */
bool util_pio_interrupt_wait_timeout(
    PIO const pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end);

/**
 * @brief Waits for a given PIO to be set within the timeout
 * period and then clears it.
 * 
 * @param pio 
 * @param pio_interrupt_num 
 * @param end
 * @return true if the interrupt was set within the timeout
 * @return false if the timeout was reached
 */
bool util_pio_interrupt_wait_clear_timeout(
    PIO const pio,
    const uint pio_interrupt_num,
    const absolute_time_t* const end);

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
bool util_pio_sm_try_get(
    PIO const pio,
    const uint sm,
    uint32_t* const word,
    const uint threshold);

#ifdef __cplusplus
}
#endif

#endif
