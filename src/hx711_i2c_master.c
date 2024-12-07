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
#include <pico/binary_info.h>
#include <pico/error.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/hx711.h"
#include "../include/hx711_i2c_master.h"
#include "../include/util.h"

#include <stdlib.h>
#include <stdio.h>
#include <pico/stdio.h>

void hx711_i2c_control_set_ready(
    const bool val,
    uint8_t* const control) {

        assert(control != NULL);

        const uint8_t mask = ((
            ((uint8_t)val) <<
                HX711_I2C_CONTROL_READY_OFFSET) &
                HX711_I2C_CONTROL_READY_SIZE);

        *control |= mask;

}

void hx711_i2c_control_set_new_value(
    const bool is_new,
    uint8_t* const control) {

        assert(control != NULL);

        const uint8_t mask = ((
            ((uint8_t)is_new) <<
                HX711_I2C_CONTROL_NEW_VALUE_OFFSET) &
                HX711_I2C_CONTROL_NEW_VALUE_SIZE);

        *control |= mask;

}

void hx711_i2c_control_set_power(
    const bool state,
    uint8_t* const control) {

        assert(control != NULL);

        const uint8_t mask = ((
            ((uint8_t)state) <<
                HX711_I2C_CONTROL_POWER_STATE_OFFSET) &
                HX711_I2C_CONTROL_POWER_STATE_SIZE);

        *control |= mask;

}

void hx711_i2c_control_set_gain(
    const hx711_gain_t gain,
    uint8_t* const control) {

        assert(control != NULL);
        assert(hx711_is_gain_valid(gain));

        const uint8_t mask = ((
            ((uint8_t)gain) <<
                HX711_I2C_CONTROL_GAIN_OFFSET) &
                HX711_I2C_CONTROL_GAIN_SIZE);

        *control |= mask;

}

void hx711_i2c_control_set_rate(
    const hx711_rate_t rate,
    uint8_t* const control) {

        assert(control != NULL);
        assert(hx711_is_rate_valid(rate));

        const uint8_t mask = ((
            ((uint8_t)rate) <<
                HX711_I2C_CONTROL_RATE_OFFSET) &
                HX711_I2C_CONTROL_RATE_SIZE);

        *control |= mask;

}

bool hx711_i2c_control_get_ready(
    const uint8_t control) {
        return (bool)((
            control >>
            HX711_I2C_CONTROL_READY_OFFSET) &
            HX711_I2C_CONTROL_READY_SIZE);
}

bool hx711_i2c_control_get_new_value(
    const uint8_t control) {
        return (bool)((
            control >>
            HX711_I2C_CONTROL_NEW_VALUE_OFFSET) &
            HX711_I2C_CONTROL_NEW_VALUE_SIZE);
}

bool hx711_i2c_control_get_power_state(
    const uint8_t control) {
        return (bool)((
            control >>
            HX711_I2C_CONTROL_POWER_STATE_OFFSET) &
            HX711_I2C_CONTROL_POWER_STATE_SIZE);
}

hx711_gain_t hx711_i2c_control_get_gain(
    const uint8_t control) {

        const hx711_gain_t gain = (hx711_gain_t)((
            control >>
            HX711_I2C_CONTROL_GAIN_OFFSET) &
            HX711_I2C_CONTROL_GAIN_SIZE);

        assert(hx711_is_gain_valid(gain));

        return gain;

}

hx711_rate_t hx711_i2c_control_get_rate(
    const uint8_t control) {

        const hx711_rate_t rate = (hx711_rate_t)((
            control >>
            HX711_I2C_CONTROL_RATE_OFFSET) &
            HX711_I2C_CONTROL_RATE_SIZE);

        assert(hx711_is_rate_valid(rate));

        return rate;

}

void hx711_i2c_command_set_command(
    const hx711_i2c_command_t cmd,
    uint8_t* const bits) {

        assert(bits != NULL);
        assert(hx711_i2c_command_is_valid(cmd));

        const uint8_t mask = ((
            ((uint8_t)cmd) << 
            HX711_I2C_COMMAND_COMMAND_OFFSET) &
            HX711_I2C_COMMAND_COMMAND_SIZE);

        *bits |= mask;

}

void hx711_i2c_command_set_power_state(
    const bool power,
    uint8_t* const bits) {

        assert(bits != NULL);

        const uint8_t mask = ((
            ((uint8_t)power) <<
            HX711_I2C_COMMAND_POWER_OFFSET) &
            HX711_I2C_COMMAND_POWER_SIZE);

        *bits |= mask;

}

void hx711_i2c_command_set_gain(
    const hx711_gain_t gain,
    uint8_t* const bits) {

        assert(bits != NULL);
        assert(hx711_is_gain_valid(gain));

        const uint8_t mask = ((
            ((uint8_t)gain) <<
            HX711_I2C_COMMAND_GAIN_OFFSET) &
            HX711_I2C_COMMAND_GAIN_SIZE);
        
        *bits |= mask;

}

void hx711_i2c_command_set_rate(
    const hx711_rate_t rate,
    uint8_t* const bits) {

        assert(bits != NULL);
        assert(hx711_is_rate_valid(rate));

        const uint8_t mask = ((
            ((uint8_t)rate) <<
            HX711_I2C_COMMAND_RATE_OFFSET) &
            HX711_I2C_COMMAND_RATE_SIZE);

        *bits |= mask;

}

hx711_i2c_command_t hx711_i2c_command_get_command(
    const uint8_t bits) {

        const hx711_i2c_command_t cmd = (hx711_i2c_command_t)(
            ((bits >>
            HX711_I2C_COMMAND_COMMAND_OFFSET) &
            HX711_I2C_COMMAND_COMMAND_SIZE));

        assert(hx711_i2c_command_is_valid(cmd));

        return cmd;

}

bool hx711_i2c_command_get_power(
    const uint8_t data) {
        return (bool)(
            ((data >>
            HX711_I2C_COMMAND_POWER_OFFSET) &
            HX711_I2C_COMMAND_POWER_SIZE));
}

hx711_gain_t hx711_i2c_command_get_gain(
    const uint8_t data) {

        const hx711_gain_t gain = (hx711_gain_t)(
            ((data >>
            HX711_I2C_COMMAND_GAIN_OFFSET) &
            HX711_I2C_COMMAND_GAIN_SIZE));

        assert(hx711_is_gain_valid(gain));

        return gain;

}

hx711_rate_t hx711_i2c_command_get_rate(
    const uint8_t data) {

        const hx711_rate_t rate = (hx711_rate_t)(
            ((data >>
            HX711_I2C_COMMAND_RATE_OFFSET) &
            HX711_I2C_COMMAND_RATE_SIZE));

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

        i2c_init(hx_i2c->_i2c, hx_i2c->_baud_rate);
        i2c_set_slave_mode(hx_i2c->_i2c, false, 0);

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

bool hx711_i2c_master_get_value(
    hx711_i2c_master_t* const hx_i2c,
    int32_t* const val) {

        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);

        uint8_t inbuff[HX711_I2C_CONTROL_TOTAL_BYTES];

        const int bytesRead = i2c_read_blocking(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            inbuff,
            HX711_I2C_CONTROL_TOTAL_BYTES,
            true);

        if(bytesRead == PICO_ERROR_GENERIC) {
            printf("Generic error\n");
            return false;
        }
        else if(bytesRead == PICO_ERROR_TIMEOUT) {
            printf("Timeout\n");
            return false;
        }
        else if(bytesRead != 4) {
            printf("Bytes read %i\n", bytesRead);
            return false;
        }

        *val = hx711_i2c_array_to_value(
            &inbuff[HX711_I2C_CONTROL_DATA_OFFSET_BYTES]);

        //HX711_I2C_PRINT_CONTROL(inbuff[HX711_I2C_CONTROL_METADATA_OFFSET_BYTES]);

        return hx711_i2c_control_get_new_value(
            inbuff[HX711_I2C_CONTROL_METADATA_OFFSET_BYTES]);

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
            hx711_i2c_command_change_power,
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
            hx711_i2c_command_change_power,
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

uint8_t hx711_i2c_generate_checksum(
    const uint8_t* const arr, const size_t len) {
    uint8_t crc = 0x01; // Initial CRC value (non-zero)

    for (size_t i = 0; i < len; ++i) {
        crc ^= arr[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07; // Polynomial: x^8 + x^2 + x^1 + x^0
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void hx711_i2c_value_to_array(
    const int32_t val,
    uint8_t* const arr) {
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

        val = (val << 8) >> 8;

        return val;

}

bool hx711_i2c_is_i2c_gain_valid(const uint8_t i2c_gain) {
    return i2c_gain <= count_of(HX711_CLOCK_PULSES) - 1;
}

uint8_t hx711_i2c_gain_to_i2c_gain(const hx711_gain_t gain) {
    assert(hx711_is_gain_valid(gain));
    return (uint8_t)gain - HX711_READ_BITS;
}

hx711_gain_t hx711_i2c_i2c_gain_to_gain(const uint8_t i2c_gain) {
    assert(hx711_i2c_is_i2c_gain_valid(i2c_gain));
    const hx711_gain_t gain = (hx711_gain_t)HX711_CLOCK_PULSES[i2c_gain];
    assert(hx711_is_gain_valid(gain));
    return gain;
}

bool hx711_i2c_is_command_valid(
    const hx711_i2c_command_t cmd) {
        return (uint8_t)cmd <= hx711_i2c_command_get_value;
}
