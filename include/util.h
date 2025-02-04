// MIT License
// 
// Copyright (c) 2024 Daniel Robertson
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
#include <hardware/dma.h>
#include <hardware/pio.h>
#include <hardware/platform_defs.h>
#include <hardware/spi.h>
#include <hardware/sync.h>
#include <pico/mutex.h>
#include <pico/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UTIL_RETURNIF(COND, VALUE) if(COND) return (VALUE)
#define UTIL_BREAKIF(COND) if(COND) break
#define UTIL_CONTINUEIF(COND) if(COND) continue

// https://stackoverflow.com/a/3208376
#define UTIL_BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define UTIL_BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

// define these since they're not included by default
// and they're helpful
#ifndef UINT8_WIDTH
#define UINT8_WIDTH                         8u
#endif

#ifndef UINT16_WIDTH
#define UINT16_WIDTH                        16u
#endif

#ifndef UINT32_WIDTH
#define UINT32_WIDTH                        32u
#endif

#define UTIL__DECL_IN_RANGE_FUNC(TYPE) \
bool util_ ## TYPE ##_in_range( \
    const TYPE val, \
    const TYPE min, \
    const TYPE max);

#define UTIL__DEF_IN_RANGE_FUNC(TYPE) \
inline bool util_ ## TYPE ##_in_range( \
    const TYPE val, \
    const TYPE min, \
    const TYPE max) { \
        return val >= min && val <= max; \
}

UTIL__DEF_IN_RANGE_FUNC(int32_t)
UTIL__DEF_IN_RANGE_FUNC(uint32_t)
UTIL__DEF_IN_RANGE_FUNC(int)
UTIL__DEF_IN_RANGE_FUNC(uint)
UTIL__DEF_IN_RANGE_FUNC(size_t)
#undef UTIL__DECL_IN_RANGE_FUNC
#undef UTIL__DEF_IN_RANGE_FUNC

#define UTIL__DECL_GET_BITS_FUNC(BIT_COUNT) \
uint ## BIT_COUNT ##_t util_get_bits ## BIT_COUNT ( \
    const uint ## BIT_COUNT ##_t value, \
    const uint ## BIT_COUNT ##_t startbit, \
    const uint ## BIT_COUNT ##_t len);

#define UTIL__DECL_SET_BITS_FUNC(BIT_COUNT) \
uint ## BIT_COUNT ##_t util_set_bits ## BIT_COUNT ( \
    uint ## BIT_COUNT ##_t value, \
    const uint ## BIT_COUNT ##_t startbit, \
    const uint ## BIT_COUNT ##_t len, \
    const uint ## BIT_COUNT ##_t bits);

#define UTIL__DEF_GET_BITS_FUNC(BIT_COUNT) \
inline uint ## BIT_COUNT ##_t util_get_bits ## BIT_COUNT ( \
    const uint ## BIT_COUNT ##_t value, \
    const uint ## BIT_COUNT ##_t startbit, \
    const uint ## BIT_COUNT ##_t len) { \
        \
        assert(util_uint_in_range(startbit, 0, UINT ## BIT_COUNT ##_WIDTH - 1)); \
        assert(util_uint_in_range(len, 1, UINT ## BIT_COUNT ##_WIDTH)); \
        assert((startbit + len) <= UINT ## BIT_COUNT ##_WIDTH); \
        \
        const uint ## BIT_COUNT ##_t mask = ((1 << len) - 1) << startbit; \
        const uint ## BIT_COUNT ##_t extracted = (value & mask) >> startbit; \
        \
        return extracted; \
        \
}

#define UTIL__DEF_SET_BITS_FUNC(BIT_COUNT) \
inline uint ## BIT_COUNT ##_t util_set_bits ## BIT_COUNT ( \
    uint ## BIT_COUNT ##_t value, \
    const uint ## BIT_COUNT ##_t startbit, \
    const uint ## BIT_COUNT ##_t len, \
    const uint ## BIT_COUNT ##_t bits) { \
        \
        assert(util_uint_in_range(startbit, 0, UINT ## BIT_COUNT ##_WIDTH - 1)); \
        assert(util_uint_in_range(len, 1, UINT ## BIT_COUNT ##_WIDTH)); \
        assert((startbit + len) <= UINT ## BIT_COUNT ##_WIDTH); \
        \
        const uint ## BIT_COUNT ##_t mask = ((1 << len) - 1) << startbit; \
        value &= ~mask; \
        value |= (bits << startbit); \
        return value; \
        \
}

UTIL__DEF_GET_BITS_FUNC(8)
UTIL__DEF_GET_BITS_FUNC(16)
UTIL__DEF_GET_BITS_FUNC(32)
#undef UTIL__DECL_GET_BITS_FUNC
#undef UTIL__DEF_GET_BITS_FUNC

UTIL__DEF_SET_BITS_FUNC(8)
UTIL__DEF_SET_BITS_FUNC(16)
UTIL__DEF_SET_BITS_FUNC(32)
#undef UTIL__DECL_SET_BITS_FUNC
#undef UTIL__DEF_SET_BITS_FUNC

// NUM_DMA_IRQS defined in pico sdk
#define UTIL_NUM_DMA_IRQS                   NUM_DMA_IRQS

#define UTIL_DMA_IRQ_INDEX_MIN              0u
#define UTIL_DMA_IRQ_INDEX_MAX              (UTIL_NUM_DMA_IRQS - 1)

// NUM_PIOS defined in pico sdk
#define UTIL_PIO_IRQ_INDEX_MIN              0u
#define UTIL_PIO_IRQ_INDEX_MAX              (NUM_PIOS - 1)

// PIO interrupt source (pio.h)
#define UTIL_PIO_PIS_MIN                    PIO_INTR_SM0_LSB

// see typedef enum pio_interrupt_source
#if PICO_PIO_VERSION == 0
    #define UTIL_PIO_PIS_MAX                PIO_INTR_SM3_LSB
#elif PICO_PIO_VERSION == 1
    #define UTIL_PIO_PIS_MAX                PIO_INTR_SM7_LSB
#endif

#define UTIL_PIO_INTERRUPT_NUM_MIN          0u
#define UTIL_PIO_INTERRUPT_NUM_MAX          7u

#define UTIL_ROUTABLE_PIO_INTERRUPT_NUM_MIN 0u
#define UTIL_ROUTABLE_PIO_INTERRUPT_NUM_MAX 3u

/**
 * @brief Own a mutex for the duration of this block of
 * code.
 * @example UTIL_MUTEX_BLOCK(mut, 
 *  // do something...
 * );
 */
#define UTIL_MUTEX_BLOCK(mut, ...) \
    do { \
        mutex_enter_blocking(&mut); \
        __VA_ARGS__ \
        mutex_exit(&mut); \
    } while(0)

/**
 * Adapted from https://stackoverflow.com/a/17624752
 */
#define UTIL__CONCAT(A, B) UTIL__CONCAT_INNER(A, B)
#define UTIL__CONCAT_INNER(A, B) A ## B
#define UTIL__UNIQUE_NAME(PREFIX) UTIL__CONCAT(PREFIX, __COUNTER__)
#define UTIL__INTERRUPTS_OFF_BLOCK_INNER(VAR, CODE) \
    do { \
        const uint32_t VAR = save_and_disable_interrupts(); \
        CODE \
        restore_interrupts(VAR); \
    } \
    while(0)

/**
 * @brief Disable interrupts for the duration of this block of
 * code.
 * @example UTIL_INTERRUPTS_OFF_BLOCK(
 *  // do something atomically...
 * );
 * @note the interrupt_status var is uniquely named to avoid
 * variable conflicts within the block.
 */
#define UTIL_INTERRUPTS_OFF_BLOCK(...) \
    do { \
        UTIL__INTERRUPTS_OFF_BLOCK_INNER( \
            UTIL__UNIQUE_NAME(interrupt_status_cb918069_eadf_49bc_9d8c_a8a4defad20c_), \
            __VA_ARGS__); \
    } \
    while(0)

/**
 * @brief Quick lookup for finding an NVIC IRQ number
 * for a PIO and interrupt index number.
 * @note Each PIO has two interrupts, hence why this
 * array is doubled.
 */
extern const uint8_t util_pio_to_irq_map[NUM_PIOS * 2];

/**
 * @brief Quick lookup for finding an NVIC IRQ number
 * for a DMA interrupt index number.
 */
extern const uint8_t util_dma_to_irq_map[UTIL_NUM_DMA_IRQS];

uint8_t util_crc8(
    uint8_t data,
    const uint8_t polynomial);

uint32_t util_crc32(
    const uint8_t* const data,
    const size_t len,
    const uint32_t polynomial);

/**
 * @brief Check whether a DMA IRQ index is valid.
 * 
 * @param idx 
 * @return true 
 * @return false 
 */
inline bool util_dma_irq_index_is_valid(
    const uint idx) {
        return util_uint_in_range(
            idx,
            UTIL_DMA_IRQ_INDEX_MIN,
            UTIL_DMA_IRQ_INDEX_MAX);
}

/**
 * @brief Gets the NVIC DMA IRQ number using the DMA
 * IRQ index.
 * 
 * @param idx 
 * @return uint 
 */
uint util_dma_get_irq_from_index(
    const uint idx);

/**
 * @brief Gets the DMA IRQ index using the NVIC IRQ
 * number.
 * 
 * @param irq_num 
 * @return int -1 is returned for no match.
 */
int util_dma_get_index_from_irq(
    const uint irq_num);

/**
 * @brief Set and enable an exclusive handler for a
 * DMA channel.
 * 
 * @param irq_index 
 * @param channel 
 * @param handler 
 * @param enabled 
 */
void util_dma_set_exclusive_channel_irq_handler(
    const uint irq_index,
    const uint channel,
    const irq_handler_t handler,
    const bool enabled);

/**
 * @brief Get the transfer count for a given DMA channel. When a
 * DMA transfer is active, this count is the number of transfers
 * remaining.
 * 
 * @param channel 
 * @return uint32_t 
 */
uint32_t util_dma_get_transfer_count(
    const uint channel);

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
 * @param irq_index 0 or 1
 * @return uint DMA_IRQ_0 or DMA_IRQ_1
 */
uint util_dma_get_irqn(
    const uint irq_index);

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
void util_gpio_set_output(
    const uint gpio);

/**
 * @brief Set and enable an exclusive interrupt handler
 * for a given pio_interrupt_num.
 * 
 * @param pio 
 * @param irq_index 
 * @param pio_interrupt_num 
 * @param handler 
 * @param enabled 
 */
void util_irq_set_exclusive_pio_interrupt_num_handler(
    PIO const pio,
    const uint irq_index,
    const uint pio_interrupt_num,
    const irq_handler_t handler,
    const bool enabled);

/**
 * @brief Check whether PIO IRQ index is valid
 * 
 * @param idx 
 * @return true 
 * @return false 
 */
inline bool util_pio_irq_index_is_valid(
    const uint idx) {
        return util_uint_in_range(
            idx,
            UTIL_PIO_IRQ_INDEX_MIN,
            UTIL_PIO_IRQ_INDEX_MAX);
}

/**
 * @brief Gets the NVIC PIO IRQ number using a PIO
 * pointer and PIO IRQ index.
 * 
 * @param idx 
 * @return uint 
 */
uint util_pio_get_irq_from_index(
    PIO const pio,
    const uint idx);

/**
 * @brief Gets the PIO IRQ index using the NVIC IRQ
 * number.
 * 
 * @param irq_num 
 * @return int -1 is returned for no match.
 */
int util_pio_get_index_from_irq(
    const uint irq_num);

/**
 * @brief Gets the PIO using the NVIC IRQ number.
 * 
 * @param irq_num 
 * @return PIO const 
 */
PIO const util_pio_get_pio_from_irq(
    const uint irq_num);

/**
 * @brief Gets the correct NVIC IRQ number for a PIO
 * according to the IRQ index.
 * 
 * @example util_pion_get_irqn(pio1, 1); //returns PIO1_IRQ_1
 * 
 * @param pio 
 * @param irq_index 0 or 1
 * @return uint 
 */
uint util_pion_get_irqn(
    PIO const pio,
    const uint irq_index);

/**
 * @brief Gets the correct PIO interrupt source number according
 * to the raw PIO interrupt number used in a .pio file.
 * 
 * @example util_pio_get_pis_from_pio_interrupt_num(3); //returns pis_interrupt3 (11)
 * 
 * @see pio_interrupt_source
 * @param pio_interrupt_num 
 * @return uint 
 */
uint util_pio_get_pis_from_pio_interrupt_num(
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
 * @brief Clears a given state machine's OSR.
 * 
 * @param pio 
 * @param sm 
 */
void util_pio_sm_clear_osr(
    PIO const pio,
    const uint sm);

/**
 * @brief Clears a given state machine's ISR.
 * 
 * @param pio 
 * @param sm 
 */
void util_pio_sm_clear_isr(
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

/**
 * @brief Check whether a PIO interrupt number is valid.
 * 
 * @param pio_interrupt_num 
 * @return true 
 * @return false 
 */
inline bool util_pio_interrupt_num_is_valid(
    const uint pio_interrupt_num) {
        return util_uint_in_range(
            pio_interrupt_num,
            UTIL_PIO_INTERRUPT_NUM_MIN,
            UTIL_PIO_INTERRUPT_NUM_MAX);
}

/**
 * @brief Check whether a PIO interrupt number is
 * a valid routable interrupt number.
 * 
 * @param pio_interrupt_num 
 * @return true 
 * @return false 
 */
inline bool util_routable_pio_interrupt_num_is_valid(
    const uint pio_interrupt_num) {
        return util_uint_in_range(
            pio_interrupt_num,
            UTIL_ROUTABLE_PIO_INTERRUPT_NUM_MIN,
            UTIL_ROUTABLE_PIO_INTERRUPT_NUM_MAX);
}

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

bool util_spi_is_readable_timeout(
    const spi_inst_t* const spi,
    const absolute_time_t* const timeout);

bool util_spi_is_writable_timeout(
    const spi_inst_t* const spi,
    const absolute_time_t* const timeout);

#ifdef __cplusplus
}
#endif

#endif
