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

#include <assert.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <pico/error.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/hx711.h"
#include "../include/hx711_remote.h"
#include "../include/hx711_i2c_master.h"
#include "../include/util.h"

void hx711_i2c_master_init(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_i2c_master_config_t* const hx_i2c_config) {

        assert(hx_i2c != NULL);
        assert(hx_i2c_config != NULL);

        assert(hx_i2c_config->i2c != NULL);
        assert(hx_i2c_config->baud_rate > 0);

        hx_i2c->_scl_pin = hx_i2c_config->scl_pin;
        hx_i2c->_sda_pin = hx_i2c_config->sda_pin;
        hx_i2c->_i2c = hx_i2c_config->i2c;
        hx_i2c->_baud_rate = hx_i2c_config->baud_rate;
        hx_i2c->_addr = hx_i2c_config->addr;

        gpio_init(hx_i2c->_scl_pin);
        gpio_init(hx_i2c->_sda_pin);

        gpio_set_dir(hx_i2c->_scl_pin, true);
        gpio_set_dir(hx_i2c->_sda_pin, false);

        gpio_set_function(hx_i2c->_scl_pin, GPIO_FUNC_I2C);
        gpio_set_function(hx_i2c->_sda_pin, GPIO_FUNC_I2C);

        gpio_pull_up(hx_i2c->_scl_pin);
        gpio_pull_up(hx_i2c->_sda_pin);

        i2c_init(
            hx_i2c->_i2c,
            hx_i2c->_baud_rate);

        i2c_set_slave_mode(
            hx_i2c->_i2c,
            false,
            0);

}

void hx711_i2c_master_close(
    hx711_i2c_master_t* const hx_i2c) {
        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);
        i2c_deinit(hx_i2c->_i2c);
}

int hx711_i2c_master_set_gain(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_gain_t gain,
    const hx711_rate_t rate) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);
        assert(hx711_is_gain_valid(gain));
        assert(hx711_is_rate_valid(rate));

        const hx711_remote_request_t req = {
            .cmd = hx711_remote_command_change_gain,
            .gain = gain,
            .rate = rate
        };

        uint8_t buffer[HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES];

        hx711_remote_request_to_buffer(
            &req,
            buffer);

        return i2c_write_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            buffer,
            sizeof(buffer),
            true);

}

int hx711_i2c_master_get_control(
    hx711_i2c_master_t* const hx_i2c,
    hx711_remote_control_t* const ctrl) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);
        assert(control != NULL);

        uint8_t buffer[HX711_REMOTE_CONTROL_TOTAL_BYTES];

        const int bytesRead = i2c_read_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            buffer,
            HX711_REMOTE_CONTROL_TOTAL_BYTES,
            true);

        if(bytesRead != HX711_REMOTE_CONTROL_TOTAL_BYTES) {
            // eg. incorrect number of bytes received
            return bytesRead;
        }

        hx711_remote_buffer_to_control(
            buffer,
            ctrl);

        return PICO_OK;

}

int32_t hx711_i2c_master_get_value_blocking(
    hx711_i2c_master_t* const hx_i2c) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);

        hx711_remote_control_t ctrl;

        while(hx711_i2c_master_get_control(hx_i2c, &ctrl) != PICO_OK) {
            if(!hx711_remote_control_ok(&ctrl)) {
                continue;
            }
        }

        return ctrl.value;

}

int hx711_i2c_master_power_up(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_gain_t gain,
    const hx711_rate_t rate) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);
        assert(hx711_is_gain_valid(gain));
        assert(hx711_is_rate_valid(rate));

        const hx711_remote_request_t req = {
            .cmd = hx711_remote_command_change_power_state,
            .power_state = true,
            .gain = gain,
            .rate = rate
        };

        uint8_t buffer[HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES];

        hx711_remote_request_to_buffer(
            &req,
            buffer);

        return i2c_write_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            buffer,
            sizeof(buffer),
            true);

}

int hx711_i2c_master_power_down(
    hx711_i2c_master_t* const hx_i2c) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);

        const hx711_remote_request_t req = {
            .cmd = hx711_remote_command_change_power_state,
            .power_state = false
        };

        uint8_t buffer[HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES];

        hx711_remote_request_to_buffer(
            &req,
            buffer);

        return i2c_write_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            buffer,
            sizeof(buffer),
            true);

}
