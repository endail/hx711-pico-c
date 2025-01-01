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

#include <assert.h>
#include <hardware/i2c.h>
#include <pico/i2c_slave.h>
#include <pico/platform.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "hx711_remote.h"
#include "hx711_i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Number of slaves supported. Arbitrary, but exists
 * to have a fixed sized array.
 */
#define HX711_I2C_SLAVE_MAP_SIZE                        4u

typedef struct {
    uint _scl_pin;
    uint _sda_pin;
    i2c_inst_t* _i2c;
    uint _baud_rate;
    uint8_t _addr;
    hx711_t* _hx;
    hx711_remote_control_t _memory;
    hx711_remote_request_t _inreq;
    bool _updating;
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

static bool hx711_i2c__slave_add_slave(
    hx711_i2c_slave_t* const slave);

static void hx711_i2c__slave_remove_slave(
    const hx711_i2c_slave_t* const slave);

static bool hx711_i2c__slave_get_slave(
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

inline void hx711_i2c_slave_get_control(
    hx711_i2c_slave_t* const hx_i2c,
    hx711_remote_control_t* const ctrl) {
        assert(hx_i2c != NULL);
        assert(ctrl != NULL);
        memcpy(ctrl, &hx_i2c->_memory, sizeof(ctrl));
}

inline void hx711_i2c_slave_set_control(
    hx711_i2c_slave_t* const hx_i2c,
    const hx711_remote_control_t* const ctrl) {
        assert(hx_i2c != NULL);
        assert(ctrl != NULL);
        assert(hx711_is_gain_valid(ctrl->gain));
        assert(hx711_is_rate_valid(ctrl->rate));
        memcpy(&hx_i2c->_memory, ctrl, sizeof(hx_i2c->_memory));
}

/**
 * @brief Stop i2c communication.
 * 
 * @param hx_i2c 
 */
void hx711_i2c_slave_close(
    hx711_i2c_slave_t* const hx_i2c);

static void __isr __not_in_flash_func(hx711_i2c_slave_handler)(
    i2c_inst_t* i2c,
    i2c_slave_event_t event);

/**
 * @brief Change updating state. If not currently
 * updating, call hx711_i2c_slave_update_loop after
 * setting with this function.
 * 
 * @param hx_i2c 
 * @param updating 
 */
inline void hx711_i2c_slave_set_updating(
    hx711_i2c_slave_t* const hx_i2c,
    const bool updating) {
        assert(hx_i2c != NULL);
        hx_i2c->_updating = updating;
}

/**
 * @brief Looping function to update slave values.
 * 
 * @param hx_i2c 
 */
void hx711_i2c_slave_update_loop(
    hx711_i2c_slave_t* const hx_i2c);

#ifdef __cplusplus
}
#endif

#endif