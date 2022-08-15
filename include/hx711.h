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

#ifndef _HX711_H_0ED0E077_8980_484C_BB94_AF52973CDC09
#define _HX711_H_0ED0E077_8980_484C_BB94_AF52973CDC09

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint HX711_READ_BITS = 24;

static const int32_t HX711_MIN_VALUE = -0x800000; //-pow(2, HX711_READ_BITS - 1) == -8388608

static const int32_t HX711_MAX_VALUE = 0x7fffff; //pow(2, HX711_READ_BITS - 1) - 1 == 8388607

static const uint HX711_POWER_DOWN_TIMEOUT = 60; //us

static const uint HX711_SETTLING_TIMES[] = { //ms
    400,
    50
};

typedef enum {
    hx711_rate_10 = 0,
    hx711_rate_80
} hx711_rate_t;

static const uint HX711_SAMPLE_RATES[] = {
    10,
    80
};

typedef enum {
    hx711_gain_128 = 25, //clock pulse counts
    hx711_gain_32 = 26,
    hx711_gain_64 = 27
} hx711_gain_t;

typedef enum {
    hx711_pwr_up = 0,
    hx711_pwr_down
} hx711_power_t;

typedef struct {

    uint clock_pin;
    uint data_pin;

    PIO _pio;
    const pio_program_t* _prog;
    pio_sm_config _default_config;
    uint _state_mach;
    uint _offset;

    mutex_t _mut;

} hx711_t;

/**
 * @brief Prototype for init function in .pio file.
 */
typedef void (*hx711_program_init_t)(hx711_t* const);

/**
 * @brief Initialise HX711.
 * 
 * @param hx Pointer to hx711_t
 * @param clk GPIO pin connected to HX711 CLK pin
 * @param dat GPIO pin connected to HX711 DAT pin
 * @param pio RP2040 PIO pio0 or pio1
 * @param prog PIO program
 * @param prog_init_func PIO initalisation function
 */
void hx711_init(
    hx711_t* const hx,
    const uint clk,
    const uint dat,
    PIO const pio,
    const pio_program_t* const prog,
    hx711_program_init_t prog_init_func);

/**
 * @brief Stop communication with HX711.
 * 
 * @param hx 
 */
void hx711_close(hx711_t* const hx);

/**
 * @brief Sets HX711 gain.
 * 
 * @param hx 
 * @param gain 
 */
void hx711_set_gain(
    hx711_t* const hx,
    const hx711_gain_t gain);

/**
 * @brief Convert a raw value from the HX711 to a 32-bit signed int.
 * 
 * @param raw 
 * @return int32_t 
 */
static inline int32_t hx711_get_twos_comp(const uint32_t raw) {

    const int32_t val = 
        (int32_t)(-(raw & 0x800000)) + (int32_t)(raw & 0x7fffff);

    assert(val >= HX711_MIN_VALUE && val <= HX711_MAX_VALUE);

    return val;

}

/**
 * @brief Returns true if the load cell is saturated at its
 * minimum level.
 * 
 * @param val 
 * @return true 
 * @return false 
 */
static inline bool hx711_is_min_saturated(const int32_t val) {
    return val == HX711_MIN_VALUE; //−8,388,608
}

/**
 * @brief Returns true if the load cell is saturated at its
 * maximum level.
 * 
 * @param val 
 * @return true 
 * @return false 
 */
static inline bool hx711_is_max_saturated(const int32_t val) {
    return val == HX711_MAX_VALUE; //8,388,607
}

/**
 * @brief Returns the number of milliseconds to wait according
 * to the given HX711 sample rate to allow readings to settle.
 * 
 * @param rate 
 * @return uint 
 */
static inline uint hx711_get_settling_time(const hx711_rate_t rate) {
    return HX711_SETTLING_TIMES[(uint)rate];
}

/**
 * @brief Returns the numeric sample rate of the given rate.
 * 
 * @param rate 
 * @return uint 
 */
static inline uint hx711_get_rate_sps(const hx711_rate_t rate) {
    return HX711_SAMPLE_RATES[(uint)rate];
}

/**
 * @brief Obtains a value from the HX711. Blocks until a value
 * is available.
 * 
 * @param hx 
 * @return int32_t 
 */
int32_t hx711_get_value(hx711_t* const hx);

/**
 * @brief Obtains a value from the HX711. Blocks until a value
 * is available or the timeout is reached.
 * 
 * @param hx 
 * @param timeout maximum time to wait for a value in microseconds
 * @param val pointer to the value
 * @return true if a value was obtained within the timeout
 * @return false if a timeout was reached
 */
bool hx711_get_value_timeout(
    hx711_t* const hx,
    const uint timeout,
    int32_t* const val);

/**
 * @brief Obtains a value from the HX711. Returns immediately if
 * no value is available.
 * 
 * @param hx 
 * @param val pointer to the value
 * @return true if a value was available and val is set
 * @return false if a value was not available
 */
bool hx711_get_value_noblock(
    hx711_t* const hx,
    int32_t* const val);

/**
 * @brief Changes the power state of the HX711
 * 
 * @param hx 
 * @param pwr 
 */
void hx711_set_power(
    hx711_t* const hx,
    const hx711_power_t pwr);

/**
 * @brief Convenience function for sleeping for the
 * appropriate amount of time according to the given sample
 * rate to allow readings to settle.
 * 
 * @param rate 
 */
static inline void hx711_wait_settle(const hx711_rate_t rate) {
    sleep_ms(hx711_get_settling_time(rate));
}

/**
 * @brief Convenience function for sleeping for the
 * appropriate amount of time to allow the HX711 to power
 * down.
 */
static inline void hx711_wait_power_down() {
    sleep_us(HX711_POWER_DOWN_TIMEOUT);
}

/**
 * @brief Attempts to obtain a value from the PIO RX FIFO if one is available.
 * 
 * @param pio pointer to PIO
 * @param sm state machine
 * @param val pointer to raw value from HX711 to set
 * @return true if value was obtained
 * @return false if value was not obtained
 */
static inline bool hx711__try_get_value(
    PIO const pio,
    const uint sm,
    uint32_t* const val);

/**
 * @brief Asserts that the hx has been properly initialised. Only calls assert().
 * 
 * @param hx pointer to hx711_t
 */
static inline void hx711__assert_hx_initd(hx711_t* const hx) {

    assert(hx != NULL);
    assert(hx->_pio != NULL);
    assert(pio_sm_is_claimed(hx->_pio, hx->_state_mach));
    assert(mutex_is_initialized(&hx->_mut));

}

#ifdef __cplusplus
}
#endif

#endif
