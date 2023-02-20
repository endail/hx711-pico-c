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

#ifndef HX711_MULTI_H_253BF37A_8356_462B_B8F9_39E09A7193E6
#define HX711_MULTI_H_253BF37A_8356_462B_B8F9_39E09A7193E6

#include <stdint.h>
#include <strings.h>
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "pico/platform.h"
#include "hx711.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HX711_MULTI_CONVERSION_DONE_IRQ_NUM     0u
#define HX711_MULTI_DATA_READY_IRQ_NUM          4u

#define HX711_MULTI_ASYNC_REQ_COUNT             NUM_PIOS
#define HX711_MULTI_ASYNC_PIO_IRQ_IDX           0u
#define HX711_MULTI_ASYNC_DMA_IRQ_IDX           0u

#define HX711_MULTI_MIN_CHIPS                   1u
#define HX711_MULTI_MAX_CHIPS                   32u

typedef enum {
    HX711_MULTI_ASYNC_STATE_NONE = 0,
    HX711_MULTI_ASYNC_STATE_WAITING,
    HX711_MULTI_ASYNC_STATE_READING,
    HX711_MULTI_ASYNC_STATE_DONE
} hx711_multi_async_state_t;

typedef struct {

    uint _clock_pin;
    uint _data_pin_base;
    size_t _chips_len;

    PIO _pio;

    const pio_program_t* _awaiter_prog;
    pio_sm_config _awaiter_default_config;
    uint _awaiter_sm;
    uint _awaiter_offset;

    const pio_program_t* _reader_prog;
    pio_sm_config _reader_default_config;
    uint _reader_sm;
    uint _reader_offset;

    int _dma_channel;

    uint32_t _buffer[HX711_READ_BITS];

    uint _pio_irq_index;
    uint _dma_irq_index;
    volatile hx711_multi_async_state_t _async_state;

    mutex_t _mut;

} hx711_multi_t;

typedef void (*hx711_multi_pio_init_t)(hx711_multi_t* const);
typedef void (*hx711_multi_program_init_t)(hx711_multi_t* const);

typedef struct {

    uint clock_pin;
    uint data_pin_base;
    size_t chips_len;

    uint pio_irq_index;
    uint dma_irq_index;

    PIO pio;
    hx711_multi_pio_init_t pio_init;

    const pio_program_t* awaiter_prog;
    hx711_multi_program_init_t awaiter_prog_init;

    const pio_program_t* reader_prog;
    hx711_multi_program_init_t reader_prog_init;

} hx711_multi_config_t;

/**
 * @brief Array of hxm for ISR to access. This is a global
 * variable.
 */
extern hx711_multi_t* hx711_multi__async_request_map[
    HX711_MULTI_ASYNC_REQ_COUNT];

/**
 * @brief Whether a given hxm is the cause of the current
 * DMA IRQ.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
static bool hx711_multi__async_dma_irq_is_set(
    hx711_multi_t* const hxm);

/**
 * @brief Whether a given hxm is the cause of the current
 * PIO IRQ.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
static bool hx711_multi__async_pio_irq_is_set(
    hx711_multi_t* const hxm);

/**
 * @brief Get the hxm which caused the current DMA IRQ. Returns
 * NULL if none found.
 * 
 * @return hx711_multi_t* const 
 */
static hx711_multi_t* const hx711_multi__async_get_dma_irq_request();

/**
 * @brief Get the hxm which caused the current PIO IRQ. Returns
 * NULL if none found.
 * 
 * @return hx711_multi_t* const 
 */
static hx711_multi_t* const hx711_multi__async_get_pio_irq_request();

/**
 * @brief Triggers DMA reading; moves request state from WAITING to READING.
 * 
 * @param hxm 
 */
static void hx711_multi__async_start_dma(
    hx711_multi_t* const hxm);

/**
 * @brief Check whether an async read is currently occurring.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
static bool hx711_multi__async_is_running(
    hx711_multi_t* const hxm);

/**
 * @brief Stop any current async reads and stop listening for DMA
 * and PIO IRQs.
 * 
 * @param hxm 
 */
static void hx711_multi__async_finish(
    hx711_multi_t* const hxm);

/**
 * @brief ISR handler for PIO IRQs.
 */
static void __isr __not_in_flash_func(hx711_multi__async_pio_irq_handler)();

/**
 * @brief ISR handler for DMA IRQs.
 */
static void __isr __not_in_flash_func(hx711_multi__async_dma_irq_handler)();

/**
 * @brief Adds hxm to the request array for ISR access. Returns false
 * if no space.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
static bool hx711_multi__async_add_request(
    hx711_multi_t* const hxm);

/**
 * @brief Removes the given hxm from the request array.
 * 
 * @param hxm 
 */
static void hx711_multi__async_remove_request(
    const hx711_multi_t* const hxm);

/**
 * @brief Check whether the hxm struct has been initialised.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
static bool hx711_multi__is_initd(hx711_multi_t* const hxm);

/**
 * @brief Check whether the hxm struct has PIO State Machines
 * which are enabled.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
static bool hx711_multi__is_state_machines_enabled(
    hx711_multi_t* const hxm);

/**
 * @brief Convert an array of pinvals to regular HX711
 * values.
 * 
 * @param pinvals 
 * @param values 
 * @param len number of values to convert
 */
void hx711_multi_pinvals_to_values(
    const uint32_t* const pinvals,
    int32_t* const values,
    const size_t len);

void hx711_multi_init(
    hx711_multi_t* const hxm,
    const hx711_multi_config_t* const config);

/**
 * @brief Stop communication with all HX711s.
 * 
 * @param hxm 
 */
void hx711_multi_close(hx711_multi_t* const hxm);

/**
 * @brief Sets the HX711s' gain.
 * 
 * @param hxm 
 * @param gain 
 */
void hx711_multi_set_gain(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain);

/**
 * @brief Fill an array with one value from each HX711. Blocks
 * until values are obtained.
 * 
 * @param hxm 
 * @param values 
 */
void hx711_multi_get_values(
    hx711_multi_t* const hxm,
    int32_t* const values);

/**
 * @brief Fill an array with one value from each HX711,
 * timing out if failing to obtain values within the
 * timeout period.
 * 
 * @param hxm 
 * @param values 
 * @param timeout microseconds
 * @return true if values obtained within the timeout period
 * @return false if values not obtained within the timeout period
 */
bool hx711_multi_get_values_timeout(
    hx711_multi_t* const hxm,
    int32_t* const values,
    const uint timeout);

/**
 * @brief Start an asynchronos read.
 * 
 * @param hxm 
 */
void hx711_multi_async_start(hx711_multi_t* const hxm);

/**
 * @brief Check whether an asynchronous read is complete.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
bool hx711_multi_async_done(hx711_multi_t* const hxm);

/**
 * @brief Get the values from the last asynchronous read.
 * 
 * @param hxm 
 * @param values 
 */
void hx711_multi_async_get_values(
    hx711_multi_t* const hxm,
    int32_t* const values);

/**
 * @brief Power up each HX711 and start the internal read/write
 * functionality.
 * 
 * @related hx711_wait_settle
 * @param hxm 
 * @param gain hx711_gain_t initial gain
 */
void hx711_multi_power_up(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain);

/**
 * @brief Power down each HX711 and stop the internal read/write
 * functionlity.
 * 
 * @related hx711_wait_power_down()
 * @param hxm 
 */
void hx711_multi_power_down(hx711_multi_t* const hxm);

/**
 * @brief Attempt to synchronise all connected chips. This
 * does not include a settling time.
 * 
 * @param hxm 
 * @param gain initial gain to set to all chips
 */
void hx711_multi_sync(
    hx711_multi_t* const hxm,
    const hx711_gain_t gain);

/**
 * @brief Returns the state of each chip as a bitmask. The 0th
 * bit is the first chip, 1th bit is the second, and so on.
 * 
 * @param hxm 
 * @return uint32_t 
 */
uint32_t hx711_multi_get_sync_state(
    hx711_multi_t* const hxm);

/**
 * @brief Determines whether all chips are in sync.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
bool hx711_multi_is_syncd(
    hx711_multi_t* const hxm);

#ifdef __cplusplus
}
#endif

#endif
