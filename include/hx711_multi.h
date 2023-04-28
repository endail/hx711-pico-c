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

/**
 * @brief PIO interrupt number which is set by the reader
 * PIO State Machine when a conversion period ends. It is
 * the period of time between a conversion ending and the
 * next period beginning.
 */
#define HX711_MULTI_CONVERSION_DONE_IRQ_NUM     UINT8_C(0)

/**
 * @brief PIO interrupt number which is used between the
 * awaiter and reader PIO State Machines to indicate when
 * data is ready to be retrieved. It is not used directly
 * within the main set of code, but is used to validate
 * that the IRQ number is properly available.
 */
#define HX711_MULTI_DATA_READY_IRQ_NUM          UINT8_C(4)

/**
 * @brief Only one instance of a hx711_multi can operate
 * within a PIO. So the maximum number of concurrent
 * asynchronous read processes is limited to the numeer of
 * PIOs available.
 */
#define HX711_MULTI_ASYNC_READ_COUNT            UINT8_C(NUM_PIOS)

/**
 * @brief IRQ index defaults for PIO and DMA.
 */
#define HX711_MULTI_ASYNC_PIO_IRQ_IDX           UINT8_C(0)
#define HX711_MULTI_ASYNC_DMA_IRQ_IDX           UINT8_C(0)

/**
 * @brief Minimum number of chips to connect to a hx711_multi.
 */
#define HX711_MULTI_MIN_CHIPS                   UINT8_C(1)

/**
 * @brief The max number of chips could technically be the
 * same as the number of bits output from the PIO State Machine.
 * But this is always going to be limited by the number of GPIO
 * input pins available on the RP2040. So we take the minimum of
 * the two just in case it ever increases.
 */
#define HX711_MULTI_MAX_CHIPS                   UINT8_C(MIN(NUM_BANK0_GPIOS, 32))

/**
 * @brief State of the read as it moves through the async process.
 */
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

    uint _dma_channel;

    uint32_t _buffer[HX711_READ_BITS];

    uint _pio_irq_index;
    uint _dma_irq_index;
    volatile hx711_multi_async_state_t _async_state;

#ifndef HX711_NO_MUTEX
    mutex_t _mut;
#endif

} hx711_multi_t;

typedef void (*hx711_multi_pio_init_t)(hx711_multi_t* const);
typedef void (*hx711_multi_program_init_t)(hx711_multi_t* const);

typedef struct {

    /**
     * @brief GPIO pin number connected to all HX711 chips.
     */
    uint clock_pin;

    /**
     * @brief Lowest GPIO pin number connected to a HX711 chip.
     */
    uint data_pin_base;

    /**
     * @brief Number of HX711 chips connected.
     */
    size_t chips_len;


    /**
     * @brief Which index to use for a PIO interrupt. Either 0 or 1.
     * Corresponds to PIO[PIO_INDEX]_IRQ[IRQ_INDEX] NVIC IRQ number.
     */
    uint pio_irq_index;

    /**
     * @brief Which index to use for a DMA interrupt. Either 0 or 1.
     * Corresponds to DMA_IRQ[IRQ_INDEX] NVIC IRQ number.
     */
    uint dma_irq_index;


    /**
     * @brief Which PIO to use. Either pio0 or pio1.
     */
    PIO pio;

    /**
     * @brief PIO init function. This is called to set up any PIO
     * functions (pio_*) as opposed to any State Machine functions
     * (pio_sm_*).
     */
    hx711_multi_pio_init_t pio_init;


    /**
     * @brief PIO awaiter program.
     */
    const pio_program_t* awaiter_prog;

    /**
     * @brief PIO awaiter init function. This is called to set up
     * the State Machine for the awaiter program. It is called once
     * prior to the State Machine being enabled.
     */
    hx711_multi_program_init_t awaiter_prog_init;


    /**
     * @brief PIO reader program.
     */
    const pio_program_t* reader_prog;

    /**
     * @brief PIO reader init function. This is called to set up
     * the State Machine for the reader program. It is called once
     * prior to the State Machine being enabled.
     */
    hx711_multi_program_init_t reader_prog_init;

} hx711_multi_config_t;

/**
 * @brief Array of hxm for ISR to access. This is a global
 * variable.
 */
extern hx711_multi_t* hx711_multi__async_read_array[
    HX711_MULTI_ASYNC_READ_COUNT];

static void hx711_multi__init_asert(
    const hx711_multi_config_t* const config);

/**
 * @brief Subroutine for initialising PIO.
 * 
 * @param hxm 
 */
static void hx711_multi__init_pio(hx711_multi_t* const hxm);

/**
 * @brief Subroutine for initialising DMA.
 * 
 * @param hxm 
 */
static void hx711_multi__init_dma(hx711_multi_t* const hxm);

/**
 * @brief Subroutine for initialising IRQ.
 * 
 * @param hxm 
 */
static void hx711_multi__init_irq(hx711_multi_t* const hxm);

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
 * @brief Adds hxm to the array for ISR access. Returns false
 * if no space.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
static bool hx711_multi__async_add_reader(
    hx711_multi_t* const hxm);

/**
 * @brief Removes the given hxm from the request array.
 * 
 * @param hxm 
 */
static void hx711_multi__async_remove_reader(
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
 * @brief Start an asynchronos read. This function is not
 * mutex protected.
 * 
 * @param hxm 
 */
void hx711_multi_async_start(hx711_multi_t* const hxm);

/**
 * @brief Check whether an asynchronous read is complete.
 * This function is not mutex protected.
 * 
 * @param hxm 
 * @return true 
 * @return false 
 */
bool hx711_multi_async_done(hx711_multi_t* const hxm);

/**
 * @brief Get the values from the last asynchronous read.
 * This function is not mutex protected.
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
