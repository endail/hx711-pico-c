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

#ifndef HX711_H_0ED0E077_8980_484C_BB94_AF52973CDC09
#define HX711_H_0ED0E077_8980_484C_BB94_AF52973CDC09

#include <stdbool.h>
#include <stdint.h>
#include "hardware/pio.h"
#include "pico/mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HX711_NO_MUTEX
    #define HX711_MUTEX_BLOCK(mut, ...) \
        do { \
            mutex_enter_blocking(&mut); \
            __VA_ARGS__ \
            mutex_exit(&mut); \
        } while(0)
#else
    #define HX711_MUTEX_BLOCK(mut, ...) \
    do { \
        __VA_ARGS__ \
    } while(0)
#endif

#define HX711_READ_BITS                 UINT8_C(24)
#define HX711_POWER_DOWN_TIMEOUT        UINT8_C(60) //microseconds

#define HX711_MIN_VALUE                 INT32_C(-0x800000) //−8,388,608
#define HX711_MAX_VALUE                 INT32_C(0x7fffff) //8,388,607

#define HX711_PIO_MIN_GAIN              UINT8_C(0)
#define HX711_PIO_MAX_GAIN              UINT8_C(2)

extern const unsigned short HX711_SETTLING_TIMES[3]; //milliseconds
extern const unsigned char HX711_SAMPLE_RATES[2];
extern const unsigned char HX711_CLOCK_PULSES[3];

typedef enum {
    hx711_rate_10 = 0,
    hx711_rate_80
} hx711_rate_t;

typedef enum {
    hx711_gain_128 = 0,
    hx711_gain_32,
    hx711_gain_64
} hx711_gain_t;

typedef struct {

    uint _clock_pin;
    uint _data_pin;

    PIO _pio;
    const pio_program_t* _reader_prog;
    pio_sm_config _reader_prog_default_config;
    uint _reader_sm;
    uint _reader_offset;

#ifndef HX711_NO_MUTEX
    mutex_t _mut;
#endif

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
int32_t hx711_get_twos_comp(const uint32_t raw);

/**
 * @brief Returns true if the HX711 is saturated at its
 * minimum level.
 * 
 * @param val 
 * @return true 
 * @return false 
 */
bool hx711_is_min_saturated(const int32_t val);

/**
 * @brief Returns true if the HX711 is saturated at its
 * maximum level.
 * 
 * @param val 
 * @return true 
 * @return false 
 */
bool hx711_is_max_saturated(const int32_t val);

/**
 * @brief Returns the number of milliseconds to wait according
 * to the given HX711 sample rate to allow readings to settle.
 * 
 * @param rate 
 * @return unsigned short 
 */
unsigned short hx711_get_settling_time(const hx711_rate_t rate);

/**
 * @brief Returns the numeric sample rate of the given rate.
 * 
 * @param rate 
 * @return unsigned char 
 */
unsigned char hx711_get_rate_sps(const hx711_rate_t rate);

/**
 * @brief Returns the clock pulse count for a given gain value.
 * 
 * @param gain 
 * @return unsigned char 
 */
unsigned char hx711_get_clock_pulses(const hx711_gain_t gain);

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
 * @brief Check whether the hx struct has been initalised.
 * 
 * @param hx 
 * @return true 
 * @return false 
 */
static bool hx711__is_initd(hx711_t* const hx);

/**
 * @brief Check whether the hx struct's state machines are
 * running.
 * 
 * @param hx 
 * @return true 
 * @return false 
 */
static bool hx711__is_state_machine_enabled(hx711_t* const hx);

/**
 * @brief Check whether the given value is valid for a HX711
 * implementation.
 * 
 * @param v 
 * @return true 
 * @return false 
 */
bool hx711_is_value_valid(const int32_t v);

/**
 * @brief Check whether a given value is permitted to be
 * transmitted to a PIO State Machine to set a HX711's gain
 * according to the PIO program implementation.
 * 
 * @param g 
 * @return true 
 * @return false 
 */
bool hx711_is_pio_gain_valid(const uint32_t g);

/**
 * @brief Check whether the given rate is within the range
 * of the predefined rates.
 * 
 * @param r 
 * @return true 
 * @return false 
 */
bool hx711_is_rate_valid(const hx711_rate_t r);

/**
 * @brief Check whether the given gain is within the range
 * of the predefined gains.
 * 
 * @param g 
 * @return true 
 * @return false 
 */
bool hx711_is_gain_valid(const hx711_gain_t g);

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
void hx711_wait_settle(const hx711_rate_t rate);

/**
 * @brief Convenience function for sleeping for the
 * appropriate amount of time to allow the HX711 to power
 * down.
 */
void hx711_wait_power_down();

/**
 * @brief Convert a hx711_gain_t to a numeric value appropriate
 * for a PIO State Machine.
 * 
 * @param gain 
 * @return uint32_t 
 */
uint32_t hx711_gain_to_pio_gain(const hx711_gain_t gain);

/**
 * @brief Attempts to obtain a value from the PIO RX FIFO if one is available.
 * 
 * @param pio pointer to PIO
 * @param sm state machine
 * @param val pointer to raw value from HX711 to set
 * @return true if value was obtained
 * @return false if value was not obtained
 */
static bool hx711__try_get_value(
    PIO const pio,
    const uint sm,
    uint32_t* const val);

#ifdef __cplusplus
}
#endif

#endif
