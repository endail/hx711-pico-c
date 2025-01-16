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

#include <assert.h>
#include <hardware/i2c.h>
#include <pico.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hx711.h"
#include "hx711_remote.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HX711_I2C_DEFAULT_SCL_PIN                   PICO_DEFAULT_I2C_SCL_PIN
#define HX711_I2C_DEFAULT_SDA_PIN                   PICO_DEFAULT_I2C_SDA_PIN
#define HX711_I2C_DEFAULT_INST                      i2c_default
#define HX711_I2C_DEFAULT_BAUD_RATE                 100000u
#define HX711_I2C_DEFAULT_I2C_ADDR                  0x64

#define HX711_I2C_REMOTE_REQUEST_CRC_SIZE_BYTES     sizeof(uint8_t)
#define HX711_I2C_REMOTE_CONTROL_CRC_SIZE_BYTES     sizeof(uint32_t)

#define HX711_I2C_REMOTE_REQUEST_TOTAL_BYTES        ((HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES) + (HX711_I2C_REMOTE_REQUEST_CRC_SIZE_BYTES))
#define HX711_I2C_REMOTE_CONTROL_TOTAL_BYTES        ((HX711_REMOTE_CONTROL_TOTAL_BYTES) + (HX711_I2C_REMOTE_CONTROL_CRC_SIZE_BYTES))

#define HX711_I2C_CRC8_POLYNOMIAL                   0x07u
#define HX711_I2C_CRC32_POLYNOMIAL                  0xEDB88320u

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

void hx711_i2c_remote_request_to_buffer(
    const hx711_remote_request_t* const req,
    uint8_t* const buffer);

void hx711_i2c_remote_control_to_buffer(
    const hx711_remote_control_t* const ctrl,
    uint8_t* const buffer);

bool hx711_i2c_buffer_to_remote_request(
    const uint8_t* const buffer,
    hx711_remote_request_t* const req);

bool hx711_i2c_buffer_to_remote_control(
    const uint8_t* const buffer,
    hx711_remote_control_t* const ctrl);

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
 * @return int
 */
int hx711_i2c_master_set_gain(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_gain_t gain,
    const hx711_rate_t rate);

/**
 * @brief Requests data from the HX711. Blocks until a response
 * is available.
 * 
 * @param hx_i2c 
 * @param val 
 * @param control control values from the master
 * @return int PICO_OK if no error, otherwise PICO_ERROR_IO
 */
int hx711_i2c_master_get_control(
    hx711_i2c_master_t* const hx_i2c,
    hx711_remote_control_t* const ctrl);

/**
 * @brief Obtains a value from the HX711. Blocks until a new
 * value is available.
 * 
 * @param hx_i2c 
 * @return int32_t 
 */
int32_t hx711_i2c_master_get_value_blocking(
    hx711_i2c_master_t* const hx_i2c);

/**
 * @brief Power up the HX711 with an initial gain
 * 
 * @param hx_i2c 
 * @param gain 
 * @param rate
 * @return int
 */
int hx711_i2c_master_power_up(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_gain_t gain,
    const hx711_rate_t rate);

/**
 * @brief Power down the HX711.
 * 
 * @param hx_i2c 
 * @return int
 */
int hx711_i2c_master_power_down(
    hx711_i2c_master_t* const hx_i2c);

#ifdef __cplusplus
}
#endif

#endif
