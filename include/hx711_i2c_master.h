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

#ifndef HX711_I2C_MASTER_H_F036A30C_69C9_47D7_A521_893C48C1DFD5
#define HX711_I2C_MASTER_H_F036A30C_69C9_47D7_A521_893C48C1DFD5

#include <hardware/i2c.h>
#include <pico.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hx711.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HX711_I2C_PRINT_CONTROL(CTRL) \
    do { \
        printf("| "UTIL_BYTE_TO_BINARY_PATTERN" | Ready: %s | Power: %s | New: %s | Gain: %s | Rate: %s |", \
            UTIL_BYTE_TO_BINARY(CTRL), \
            hx711_i2c_control_get_ready_state(CTRL) ? "Yes" : "No", \
            hx711_i2c_control_get_power_state(CTRL) ? "On" : "Off", \
            hx711_i2c_control_get_new_value_state(CTRL) ? "Yes" : "No", \
            HX711_GAIN_TO_STR(hx711_i2c_control_get_gain(CTRL)), \
            HX711_RATE_TO_STR(hx711_i2c_control_get_rate(CTRL))); \
    } \
    while(0)

#define HX711_I2C_DEFAULT_SCL_PIN               PICO_DEFAULT_I2C_SCL_PIN
#define HX711_I2C_DEFAULT_SDA_PIN               PICO_DEFAULT_I2C_SDA_PIN
#define HX711_I2C_DEFAULT_INST                  i2c_default
#define HX711_I2C_DEFAULT_BAUD_RATE             100000
#define HX711_I2C_DEFAULT_I2C_ADDR              0x64

/**
 * @brief Control bits structure
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * 0th bit = ready state
 * 1th bit = new value state
 * 2nd bit = power state
 * 3rd bit = gain
 * 4th bit = gain
 * 5th bit = rate
 * 6th bit = unused
 * 7th bit = unused
 * ---------------- (byte boundary)
 * 8th .. 31th bit = value bits
 */

#define HX711_I2C_CONTROL_METADATA_OFFSET_BYTES     0
#define HX711_I2C_CONTROL_READY_STATE_OFFSET        0
#define HX711_I2C_CONTROL_NEW_VALUE_STATE_OFFSET    1
#define HX711_I2C_CONTROL_POWER_STATE_OFFSET        2
#define HX711_I2C_CONTROL_GAIN_OFFSET               3
#define HX711_I2C_CONTROL_RATE_OFFSET               5
#define HX711_I2C_CONTROL_DATA_OFFSET               8
#define HX711_I2C_CONTROL_DATA_OFFSET_BYTES         1

#define HX711_I2C_CONTROL_READY_STATE_SIZE          1
#define HX711_I2C_CONTROL_NEW_VALUE_STATE_SIZE      1
#define HX711_I2C_CONTROL_POWER_STATE_SIZE          1
#define HX711_I2C_CONTROL_GAIN_SIZE                 2
#define HX711_I2C_CONTROL_RATE_SIZE                 1

#define HX711_I2C_CONTROL_DATA_SIZE_BITS            HX711_READ_BITS
#define HX711_I2C_CONTROL_DATA_SIZE_BYTES           3
#define HX711_I2C_CONTROL_METADATA_SIZE_BITS        6
#define HX711_I2C_CONTROL_METADATA_SIZE_BYTES       1
#define HX711_I2C_CONTROL_TOTAL_BYTES               4

/**
 * @brief Command bits structure
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * 0th bit = command bit
 * 1th bit = command bit
 * 2nd bit = power state bit
 * 3rd bit = gain bit
 * 4th bit = gain bit
 * 5th bit = rate bit
 * 6th bit = unused
 * 7th bit = unused
 */

#define HX711_I2C_COMMAND_COMMAND_OFFSET            0
#define HX711_I2C_COMMAND_POWER_STATE_OFFSET        2
#define HX711_I2C_COMMAND_GAIN_OFFSET               3
#define HX711_I2C_COMMAND_RATE_OFFSET               5

#define HX711_I2C_COMMAND_COMMAND_SIZE              2
#define HX711_I2C_COMMAND_POWER_STATE_SIZE          1
#define HX711_I2C_COMMAND_GAIN_SIZE                 2
#define HX711_I2C_COMMAND_RATE_SIZE                 1

typedef enum {
    hx711_i2c_command_none =                        0,
    hx711_i2c_command_change_power_state =          1,
    hx711_i2c_command_change_gain =                 2,
    hx711_i2c_command_get_value =                   3
} hx711_i2c_command_t;

typedef struct {
    uint _scl_pin;
    uint _sda_pin;
    i2c_inst_t* _i2c;
    uint _baud_rate;
    uint8_t _addr;
} hx711_i2c_master_t;

typedef struct {
    uint scl_pin;
    uint sda_pin;
    i2c_inst_t* i2c;
    uint baud_rate;
    uint8_t addr;
} hx711_i2c_master_config_t;

/**
 * @brief Convert a 32-bit value from a HX711 to
 * a 3-byte array.
 * 
 * @param val 
 * @param arr 
 */
void hx711_i2c_value_to_array(
    const int32_t val,
    uint8_t* const arr);

/**
 * @brief Convert a 3-byte array containing a HX711
 * value to a 32-bit integer.
 * 
 * @param arr 
 * @return int32_t 
 */
int32_t hx711_i2c_array_to_value(
    const uint8_t* const arr);

void hx711_i2c_control_set_ready_state(
    const bool val,
    uint8_t* const control);

void hx711_i2c_control_set_new_value_state(
    const bool is_new,
    uint8_t* const control);

void hx711_i2c_control_set_power_state(
    const bool state,
    uint8_t* const control);

void hx711_i2c_control_set_gain(
    const hx711_gain_t gain,
    uint8_t* const control);

void hx711_i2c_control_set_rate(
    const hx711_rate_t rate,
    uint8_t* const control);

bool hx711_i2c_control_get_ready_state(
    const uint8_t control);

bool hx711_i2c_control_get_new_value_state(
    const uint8_t control);

bool hx711_i2c_control_get_power_state(
    const uint8_t control);

hx711_gain_t hx711_i2c_control_get_gain(
    const uint8_t control);

hx711_rate_t hx711_i2c_control_get_rate(
    const uint8_t control);

void hx711_i2c_command_set_command(
    const hx711_i2c_command_t cmd,
    uint8_t* const bits);

void hx711_i2c_command_set_power_state(
    const bool power,
    uint8_t* const bits);

void hx711_i2c_command_set_gain(
    const hx711_gain_t gain,
    uint8_t* const bits);

void hx711_i2c_command_set_rate(
    const hx711_rate_t rate,
    uint8_t* const bits);

hx711_i2c_command_t hx711_i2c_command_get_command(
    const uint8_t bits);

bool hx711_i2c_command_get_power_state(
    const uint8_t bits);

hx711_gain_t hx711_i2c_command_get_gain(
    const uint8_t bits);

hx711_rate_t hx711_i2c_command_get_rate(
    const uint8_t bits);

bool hx711_i2c_command_is_valid(
    const hx711_i2c_command_t cmd);

void hx711_i2c_master_init(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_i2c_master_config_t* const hx_i2c_config);

/**
 * @brief Stop i2c communication.
 * 
 * @param hx_i2c 
 */
void hx711_i2c_master_close(
    hx711_i2c_master_t* const hx_i2c);

/**
 * @brief Sets the HX711 gain.
 * 
 * @param hx_i2c 
 * @param gain 
 * @param rate 
 */
void hx711_i2c_master_set_gain(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_gain_t gain,
    const hx711_rate_t rate);

/**
 * @brief Obtains a value from the HX711. Blocks until a value
 * is available.
 * 
 * @param hx_i2c 
 * @return int 0 if no error, < 0 if PICO_ERROR_GENERIC,
 * PICO_ERROR_TIMEOUT, otherwise if > 0, length of bytes received
 */

/**
 * @brief Obtains a value from the HX711. Blocks until a value
 * is available.
 * 
 * @param hx_i2c 
 * @param val 
 * @param control control values from the master, can be NULL
 * @return int 0 if no error, < 0 if PICO_ERROR_GENERIC,
 * PICO_ERROR_TIMEOUT, otherwise if > 0, length of bytes received
 */
int hx711_i2c_master_get_value(
    hx711_i2c_master_t* const hx_i2c,
    int32_t* const val,
    uint8_t* const control);

/**
 * @brief Power up the HX711 with an initial gain
 * 
 * @param hx_i2c 
 * @param gain 
 * @param rate
 */
void hx711_i2c_master_power_up(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_gain_t gain,
    const hx711_rate_t rate);

/**
 * @brief Power down the HX711.
 * 
 * @param hx_i2c 
 */
void hx711_i2c_master_power_down(
    hx711_i2c_master_t* const hx_i2c);

#ifdef __cplusplus
}
#endif

#endif
