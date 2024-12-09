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

#ifndef HX711_I2C_SLAVE_H_9F3805FC_86F8_4A24_94B6_7493F4CDAF26
#define HX711_I2C_SLAVE_H_9F3805FC_86F8_4A24_94B6_7493F4CDAF26

#include <hardware/i2c.h>
#include <pico/i2c_slave.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stdint.h>
#include "hx711_i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Number of slaves supported. Can be altered.
 */
#define HX711_I2C_SLAVE_MAP_SIZE                        8
#define HX711_I2C_SLAVE_MEMORY_SIZE                     HX711_I2C_CONTROL_TOTAL_BYTES
#define HX711_I2C_SLAVE_DEFAULT_CONTROL_METADATA_BITS   UINT8_C(0b00100000)

typedef struct {
    uint _scl_pin;
    uint _sda_pin;
    i2c_inst_t* _i2c;
    uint _baud_rate;
    uint8_t _addr;
    hx711_t* _hx;
    volatile uint8_t _memory[HX711_I2C_SLAVE_MEMORY_SIZE];
    uint8_t _indata;
} hx711_i2c_slave_t;

typedef struct {
    uint scl_pin;
    uint sda_pin;
    i2c_inst_t* i2c;
    uint baud_rate;
    uint8_t addr;
    hx711_t* hx;
} hx711_i2c_slave_config_t;

extern hx711_i2c_slave_t* hx711_i2c__slave_map[
    HX711_I2C_SLAVE_MAP_SIZE];

bool hx711_i2c_slave_add_slave(
    hx711_i2c_slave_t* const slave);

void hx711_i2c_slave_remove_slave(
    const hx711_i2c_slave_t* const slave);

bool hx711_i2c_slave_get_slave(
    const i2c_inst_t* const i2c,
    hx711_i2c_slave_t** slave);

/**
 * @brief Initialise i2c slave device.
 * 
 * @param hx_i2c 
 * @param hx_i2c_config 
 */
void hx711_i2c_slave_init(
    hx711_i2c_slave_t* const hx_i2c,
    const hx711_i2c_slave_config_t * const hx_i2c_config);

volatile uint8_t* hx711_i2c_slave_get_control_ptr(
    hx711_i2c_slave_t* const hx_i2c);

uint8_t hx711_i2c_slave_get_control(
    const hx711_i2c_slave_t* const hx_i2c);

void hx711_i2c_slave_set_control(
    hx711_i2c_slave_t* const hx_i2c,
    const uint8_t control);

void hx711_i2c_slave_control_set_ready_state(
    hx711_i2c_slave_t* const hx_i2c,
    const bool val);

void hx711_i2c_slave_control_set_new_value_state(
    hx711_i2c_slave_t* const hx_i2c,
    const bool is_new);

void hx711_i2c_slave_control_set_power_state(
    hx711_i2c_slave_t* const hx_i2c,
    const bool state);

void hx711_i2c_slave_control_set_gain(
    hx711_i2c_slave_t* const hx_i2c,
    const hx711_gain_t gain);

void hx711_i2c_slave_control_set_rate(
    hx711_i2c_slave_t* const hx_i2c,
    const hx711_rate_t rate);

bool hx711_i2c_slave_control_get_ready_state(
    hx711_i2c_slave_t* const hx_i2c);

bool hx711_i2c_slave_control_get_new_value_state(
    hx711_i2c_slave_t* const hx_i2c);

bool hx711_i2c_slave_control_get_power_state(
    hx711_i2c_slave_t* const hx_i2c);

hx711_gain_t hx711_i2c_slave_control_get_gain(
    hx711_i2c_slave_t* const hx_i2c);

hx711_rate_t hx711_i2c_slave_control_get_rate(
    hx711_i2c_slave_t* const hx_i2c);

void hx711_i2c_slave_get_data(
    hx711_i2c_slave_t* const hx_i2c,
    uint8_t* const data);

void hx711_i2c_slave_set_data(
    hx711_i2c_slave_t* const hx_i2c,
    const uint8_t* const data);

/**
 * @brief Stop i2c communication.
 * 
 * @param hx_i2c 
 */
void hx711_i2c_slave_close(
    hx711_i2c_slave_t* const hx_i2c);

void hx711_i2c_slave_handler(
    i2c_inst_t* i2c,
    i2c_slave_event_t event);

void hx711_i2c_slave_update_loop(
    hx711_i2c_slave_t* const hx_i2c);

#ifdef __cplusplus
}
#endif

#endif