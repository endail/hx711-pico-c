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
#include "../include/hx711_i2c_master.h"
#include "../include/util.h"

void hx711_i2c_value_to_array(
    const int32_t val,
    uint8_t* const arr) {
        //TODO: ??? shifting??
        const int32_t extval = (val << 8) >> 8;
        arr[0] = (uint8_t)extval;
        arr[1] = (uint8_t)(extval >> 8);
        arr[2] = (uint8_t)(extval >> 16);
}

int32_t hx711_i2c_array_to_value(
    const uint8_t* const arr) {

        int32_t val = 
            (arr[0] << 0) |
            (arr[1] << 8) |
            (arr[2] << 16);

        //TODO: ??? shifting??
        val = (val << 8) >> 8;

        return val;

}

void hx711_i2c_control_set_ready_state(
    const bool val,
    uint8_t* const control) {
        assert(control != NULL);
        *control = util_set_bits8(
            *control,
            HX711_I2C_CONTROL_READY_STATE_OFFSET,
            HX711_I2C_CONTROL_READY_STATE_SIZE,
            (uint8_t)val);
}

void hx711_i2c_control_set_new_value_state(
    const bool is_new,
    uint8_t* const control) {
        assert(control != NULL);
        *control = util_set_bits8(
            *control,
            HX711_I2C_CONTROL_NEW_VALUE_STATE_OFFSET,
            HX711_I2C_CONTROL_NEW_VALUE_STATE_SIZE,
            (uint8_t)is_new);
}

void hx711_i2c_control_set_power_state(
    const bool state,
    uint8_t* const control) {
        assert(control != NULL);
        *control = util_set_bits8(
            *control,
            HX711_I2C_CONTROL_POWER_STATE_OFFSET,
            HX711_I2C_CONTROL_POWER_STATE_SIZE,
            (uint8_t)state);
}

void hx711_i2c_control_set_gain(
    const hx711_gain_t gain,
    uint8_t* const control) {
        assert(control != NULL);
        assert(hx711_is_gain_valid(gain));
        *control = util_set_bits8(
            *control,
            HX711_I2C_CONTROL_GAIN_OFFSET,
            HX711_I2C_CONTROL_GAIN_SIZE,
            (uint8_t)gain);
}

void hx711_i2c_control_set_rate(
    const hx711_rate_t rate,
    uint8_t* const control) {
        assert(control != NULL);
        assert(hx711_is_rate_valid(rate));
        *control = util_set_bits8(
            *control,
            HX711_I2C_CONTROL_RATE_OFFSET,
            HX711_I2C_CONTROL_RATE_SIZE,
            (uint8_t)rate);
}

bool hx711_i2c_control_get_ready_state(
    const uint8_t control) {
        return (bool)util_get_bits8(
            control,
            HX711_I2C_CONTROL_READY_STATE_OFFSET,
            HX711_I2C_CONTROL_READY_STATE_SIZE);
}

bool hx711_i2c_control_get_new_value_state(
    const uint8_t control) {
        return (bool)util_get_bits8(
            control,
            HX711_I2C_CONTROL_NEW_VALUE_STATE_OFFSET,
            HX711_I2C_CONTROL_NEW_VALUE_STATE_SIZE);
}

bool hx711_i2c_control_get_power_state(
    const uint8_t control) {
        return (bool)util_get_bits8(
            control,
            HX711_I2C_CONTROL_POWER_STATE_OFFSET,
            HX711_I2C_CONTROL_POWER_STATE_SIZE);
}

hx711_gain_t hx711_i2c_control_get_gain(
    const uint8_t control) {
        const hx711_gain_t gain = (hx711_gain_t)util_get_bits8(
            control,
            HX711_I2C_CONTROL_GAIN_OFFSET,
            HX711_I2C_CONTROL_GAIN_SIZE);
        assert(hx711_is_gain_valid(gain));
        return gain;
}

hx711_rate_t hx711_i2c_control_get_rate(
    const uint8_t control) {
        const hx711_rate_t rate = (hx711_rate_t)util_get_bits8(
            control,
            HX711_I2C_CONTROL_RATE_OFFSET,
            HX711_I2C_CONTROL_RATE_SIZE);
        assert(hx711_is_rate_valid(rate));
        return rate;
}

void hx711_i2c_command_set_command(
    const hx711_i2c_command_t cmd,
    uint8_t* const bits) {
        assert(bits != NULL);
        assert(hx711_i2c_command_is_valid(cmd));
        *bits = util_set_bits8(
            *bits,
            HX711_I2C_COMMAND_COMMAND_OFFSET,
            HX711_I2C_COMMAND_COMMAND_SIZE,
            (uint8_t)cmd);
}

void hx711_i2c_command_set_power_state(
    const bool power,
    uint8_t* const bits) {
        assert(bits != NULL);
        *bits = util_set_bits8(
            *bits,
            HX711_I2C_COMMAND_POWER_STATE_OFFSET,
            HX711_I2C_COMMAND_POWER_STATE_SIZE,
            (uint8_t)power);
}

void hx711_i2c_command_set_gain(
    const hx711_gain_t gain,
    uint8_t* const bits) {
        assert(bits != NULL);
        assert(hx711_is_gain_valid(gain));
        *bits = util_set_bits8(
            *bits,
            HX711_I2C_COMMAND_GAIN_OFFSET,
            HX711_I2C_COMMAND_GAIN_SIZE,
            (uint8_t)gain);
}

void hx711_i2c_command_set_rate(
    const hx711_rate_t rate,
    uint8_t* const bits) {
        assert(bits != NULL);
        assert(hx711_is_rate_valid(rate));
        *bits = util_set_bits8(
            *bits,
            HX711_I2C_COMMAND_RATE_OFFSET,
            HX711_I2C_COMMAND_RATE_SIZE,
            (uint8_t)rate);
}

hx711_i2c_command_t hx711_i2c_command_get_command(
    const uint8_t bits) {
        const hx711_i2c_command_t cmd = (hx711_i2c_command_t)
            util_get_bits8(
                bits,
                HX711_I2C_COMMAND_COMMAND_OFFSET,
                HX711_I2C_COMMAND_COMMAND_SIZE);
        assert(hx711_i2c_command_is_valid(cmd));
        return cmd;
}

bool hx711_i2c_command_get_power_state(
    const uint8_t data) {
        return (bool)util_get_bits8(
            data,
            HX711_I2C_COMMAND_POWER_STATE_OFFSET,
            HX711_I2C_COMMAND_POWER_STATE_SIZE);
}

hx711_gain_t hx711_i2c_command_get_gain(
    const uint8_t data) {
        const hx711_gain_t gain = (hx711_gain_t)
            util_get_bits8(
                data,
                HX711_I2C_COMMAND_GAIN_OFFSET,
                HX711_I2C_COMMAND_GAIN_SIZE);
        assert(hx711_is_gain_valid(gain));
        return gain;
}

hx711_rate_t hx711_i2c_command_get_rate(
    const uint8_t data) {
        const hx711_rate_t rate = (hx711_rate_t)
            util_get_bits8(
                data,
                HX711_I2C_COMMAND_RATE_OFFSET,
                HX711_I2C_COMMAND_RATE_SIZE);
        assert(hx711_is_rate_valid(rate));
        return rate;
}

bool hx711_i2c_command_is_valid(
    const hx711_i2c_command_t cmd) {
        return (uint8_t)cmd <= hx711_i2c_command_get_value;
}

void hx711_i2c_master_init(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_i2c_master_config_t * const hx_i2c_config) {

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

void hx711_i2c_master_set_gain(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_gain_t gain,
    const hx711_rate_t rate) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);
        assert(hx711_is_gain_valid(gain));
        assert(hx711_is_rate_valid(rate));

        uint8_t bits = 0;

        hx711_i2c_command_set_command(
            hx711_i2c_command_change_gain,
            &bits);

        hx711_i2c_command_set_gain(
            gain,
            &bits);

        hx711_i2c_command_set_rate(
            rate,
            &bits);

        i2c_write_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            &bits,
            sizeof(bits),
            true);

}

int hx711_i2c_master_get_value(
    hx711_i2c_master_t* const hx_i2c,
    int32_t* const val,
    uint8_t* const control) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);
        assert(val != NULL);
        assert(control != NULL);

        uint8_t inbuff[HX711_I2C_CONTROL_TOTAL_BYTES];

        const int bytesRead = i2c_read_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            inbuff,
            HX711_I2C_CONTROL_TOTAL_BYTES,
            true);

        if(bytesRead < 0) {
            // eg. PICO_ERROR_GENERIC, PICO_ERROR_TIMEOUT
            return bytesRead;
        }
        else if(bytesRead != HX711_I2C_CONTROL_TOTAL_BYTES) {
            // eg. incorrect number of bytes received
            return PICO_ERROR_IO;
        }

        *val = hx711_i2c_array_to_value(
            &inbuff[HX711_I2C_CONTROL_DATA_OFFSET_BYTES]);

        if(control != NULL) {
            *control = inbuff[HX711_I2C_CONTROL_METADATA_OFFSET_BYTES];
        }

        return PICO_OK;

}

int32_t hx711_i2c_master_get_value_blocking(
    hx711_i2c_master_t* const hx_i2c) {

        int32_t val;
        uint8_t ctrl;

        while(hx711_i2c_master_get_value(hx_i2c, &val, &ctrl) != PICO_OK) {
            if(!(   hx711_i2c_control_get_new_value_state(ctrl) && 
                    hx711_i2c_control_get_ready_state(ctrl) &&
                    hx711_i2c_control_get_power_state(ctrl)
            )) {
                continue;
            }
        }

        return val;

}

void hx711_i2c_master_power_up(
    hx711_i2c_master_t* const hx_i2c,
    const hx711_gain_t gain,
    const hx711_rate_t rate) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);
        assert(hx711_is_gain_valid(gain));
        assert(hx711_is_rate_valid(rate));

        uint8_t bits = 0;

        hx711_i2c_command_set_command(
            hx711_i2c_command_change_power_state,
            &bits);

        hx711_i2c_command_set_power_state(
            true,
            &bits);

        hx711_i2c_command_set_gain(
            gain,
            &bits);

        hx711_i2c_command_set_rate(
            rate,
            &bits);

        i2c_write_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            &bits,
            sizeof(bits),
            true);

}

void hx711_i2c_master_power_down(
    hx711_i2c_master_t* const hx_i2c) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);

        uint8_t bits = 0;

        hx711_i2c_command_set_command(
            hx711_i2c_command_change_power_state,
            &bits);

        hx711_i2c_command_set_power_state(
            false,
            &bits);

        i2c_write_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            &bits,
            sizeof(bits),
            true);

}
