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

#ifndef HX711_H_0ED0E077_8980_484C_BB94_AF52973CDC09
#define HX711_H_0ED0E077_8980_484C_BB94_AF52973CDC09

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "hardware/pio.h"
#include "pico/mutex.h"
#include "pico/time.h"
#include "../include/util.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
    #define CHECK_HX711_INITD(hx) \
        assert(hx != NULL); \
        assert(hx->_pio != NULL); \
        assert(pio_sm_is_claimed(hx->_pio, hx->_state_mach)); \
        assert(mutex_is_initialized(&hx->_mut));
#else
    #define CHECK_HX711_INITD(hx)
#endif

#define HX711_READ_BITS 24
#define HX711_MIN_VALUE INT32_C(-0x800000)
#define HX711_MAX_VALUE INT32_C(0x7fffff)
#define HX711_POWER_DOWN_TIMEOUT 60 //us

#define HX711_PIO_MIN_GAIN 0
#define HX711_PIO_MAX_GAIN 2

extern const uint HX711_SETTLING_TIMES[]; //ms
extern const uint HX711_SAMPLE_RATES[];

typedef enum {
    hx711_rate_10 = 0,
    hx711_rate_80
} hx711_rate_t;

typedef enum {
    hx711_gain_128 = 25, //clock pulse counts
    hx711_gain_32 = 26,
    hx711_gain_64 = 27
} hx711_gain_t;

typedef struct {

    uint _clock_pin;
    uint _data_pin;

    PIO _pio;
    const pio_program_t* _reader_prog;
    pio_sm_config _reader_prog_default_config;
    uint _reader_sm;
    uint _reader_offset;

    mutex_t _mut;

} hx711_t;

typedef void (*hx711_pio_init_t)(hx711_t* const);
typedef void (*hx711_program_init_t)(hx711_t* const);

typedef struct {

    uint clock_pin;
    uint data_pin;

    PIO pio;
    hx711_pio_init_t pio_init;

    const pio_program_t* reader_prog;
    hx711_program_init_t reader_prog_init;

} hx711_config_t;

void hx711_init(
    hx711_t* const hx,
    const hx711_config_t* const config);

/**
 * @brief Stop communication with the HX711.
 * 
 * @param hx 
 */
void hx711_close(hx711_t* const hx);

/**
 * @brief Sets the HX711 gain.
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
        (int32_t)(-(raw & +HX711_MIN_VALUE)) + (int32_t)(raw & HX711_MAX_VALUE);

    assert(val >= HX711_MIN_VALUE);
    assert(val <= HX711_MAX_VALUE);

    return val;

}

/**
 * @brief Returns true if the HX711 is saturated at its
 * minimum level.
 * 
 * @param val 
 * @return true 
 * @return false 
 */
inline bool hx711_is_min_saturated(const int32_t val) {
    return val == HX711_MIN_VALUE; //−8,388,608
}

/**
 * @brief Returns true if the HX711 is saturated at its
 * maximum level.
 * 
 * @param val 
 * @return true 
 * @return false 
 */
inline bool hx711_is_max_saturated(const int32_t val) {
    return val == HX711_MAX_VALUE; //8,388,607
}

/**
 * @brief Returns the number of milliseconds to wait according
 * to the given HX711 sample rate to allow readings to settle.
 * 
 * @param rate 
 * @return uint 
 */
inline uint hx711_get_settling_time(const hx711_rate_t rate) {
    return HX711_SETTLING_TIMES[(uint)rate];
}

/**
 * @brief Returns the numeric sample rate of the given rate.
 * 
 * @param rate 
 * @return uint 
 */
inline uint hx711_get_rate_sps(const hx711_rate_t rate) {
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
 * @param val pointer to the value
 * @param timeout maximum time to wait for a value in microseconds
 * @return true if a value was obtained within the timeout
 * @return false if a timeout was reached
 */
bool hx711_get_value_timeout(
    hx711_t* const hx,
    int32_t* const val,
    const uint timeout);

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
 * @brief Power up the HX711 and start the internal read/write
 * functionality.
 * 
 * @related hx711_wait_settle
 * @param hx 
 * @param gain hx711_gain_t initial gain
 */
void hx711_power_up(
    hx711_t* const hx,
    const hx711_gain_t gain);

/**
 * @brief Power down the HX711 and stop the internal read/write
 * functionality
 * 
 * @related hx711_wait_power_down()
 * @param hx 
 */
void hx711_power_down(hx711_t* const hx);

/**
 * @brief Convenience function for sleeping for the
 * appropriate amount of time according to the given sample
 * rate to allow readings to settle.
 * 
 * @param rate 
 */
inline void hx711_wait_settle(const hx711_rate_t rate) {
    sleep_ms(hx711_get_settling_time(rate));
}

/**
 * @brief Convenience function for sleeping for the
 * appropriate amount of time to allow the HX711 to power
 * down.
 */
inline void hx711_wait_power_down() {
    sleep_us(HX711_POWER_DOWN_TIMEOUT);
}

/**
 * @brief Convert a hx711_gain_t to a numeric value appropriate
 * for a PIO State Machine.
 * 
 * @param gain 
 * @return uint32_t 
 */
static inline uint32_t hx711__gain_to_sm_gain(const hx711_gain_t gain) {

    /**
     * gain value is 0-based and calculated by:
     * gain = clock pulses - 24 - 1
     * ie. gain of 128 is 25 clock pulses, so
     * gain = 25 - 24 - 1
     * gain = 0
     */
    const uint32_t gainVal = (uint32_t)gain - HX711_READ_BITS - 1;

    assert(gainVal <= HX711_PIO_MAX_GAIN);

    return gainVal;

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
    uint32_t* const val) {
        static const uint byteThreshold = HX711_READ_BITS / 8;
        return util_pio_sm_try_get(
            pio,
            sm,
            val,
            byteThreshold);
}

#ifdef __cplusplus
}
#endif

#endif
