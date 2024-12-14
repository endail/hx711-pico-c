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
#include <pico/i2c_slave.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/hx711.h"
#include "../include/hx711_i2c_slave.h"
#include "../include/util.h"

hx711_i2c_slave_t* hx711_i2c__slave_map[] = {
    NULL, //...
};

bool hx711_i2c_slave_add_slave(
    hx711_i2c_slave_t* const slave) {

        assert(slave != NULL);
        assert(HX711_I2C_SLAVE_MAP_SIZE > 0);
        assert(hx711_i2c__slave_map != NULL);

        for(size_t i = 0; i < HX711_I2C_SLAVE_MAP_SIZE; ++i) {
            if(hx711_i2c__slave_map[i] == NULL) {
                hx711_i2c__slave_map[i] = slave;
                return true;
            }
        }

        return false;

}

void hx711_i2c_slave_remove_slave(
    const hx711_i2c_slave_t* const slave) {

        assert(slave != NULL);
        assert(HX711_I2C_SLAVE_MAP_SIZE > 0);
        assert(hx711_i2c__slave_map != NULL);

        for(size_t i = 0; i < HX711_I2C_SLAVE_MAP_SIZE; ++i) {
            if(hx711_i2c__slave_map[i] == slave) {
                hx711_i2c__slave_map[i] = NULL;
                return;
            }
        }

}

bool hx711_i2c_slave_get_slave(
    const i2c_inst_t* const i2c,
    hx711_i2c_slave_t** slave) {

        assert(i2c != NULL);
        assert(slave != NULL);
        assert(HX711_I2C_SLAVE_MAP_SIZE > 0);
        assert(hx711_i2c__slave_map != NULL);

        for(size_t i = 0; i < HX711_I2C_SLAVE_MAP_SIZE; ++i) {
            if(hx711_i2c__slave_map[i]->_i2c == i2c) {
                *slave = hx711_i2c__slave_map[i];
                return true;
            }
        }

        return false;

}

void hx711_i2c_slave_init(
    hx711_i2c_slave_t* const hx_i2c,
    const hx711_i2c_slave_config_t* const hx_i2c_config) {

        assert(hx_i2c != NULL);
        assert(hx_i2c_config != NULL);

        assert(hx_i2c_config->i2c != NULL);
        assert(hx_i2c_config->baud_rate > 0);
        assert(hx_i2c_config->hx != NULL);

        hx_i2c->_scl_pin = hx_i2c_config->scl_pin;
        hx_i2c->_sda_pin = hx_i2c_config->sda_pin;
        hx_i2c->_i2c = hx_i2c_config->i2c;
        hx_i2c->_baud_rate = hx_i2c_config->baud_rate;
        hx_i2c->_addr = hx_i2c_config->addr;
        hx_i2c->_hx = hx_i2c_config->hx;

        gpio_init(hx_i2c->_scl_pin);
        gpio_init(hx_i2c->_sda_pin);

        gpio_set_dir(hx_i2c->_scl_pin, false);
        gpio_set_dir(hx_i2c->_sda_pin, true);

        gpio_set_function(hx_i2c->_scl_pin, GPIO_FUNC_I2C);
        gpio_set_function(hx_i2c->_sda_pin, GPIO_FUNC_I2C);

        gpio_pull_up(hx_i2c->_scl_pin);
        gpio_pull_up(hx_i2c->_sda_pin);

        hx711_i2c_slave_set_control(
            hx_i2c,
            HX711_I2C_SLAVE_DEFAULT_CONTROL_METADATA_BITS);

        hx_i2c->_indata = 0;
        hx_i2c->_updating = true;

        hx711_i2c_slave_add_slave(hx_i2c);

        i2c_init(
            hx_i2c->_i2c,
            hx_i2c->_baud_rate);

        i2c_set_slave_mode(
            hx_i2c->_i2c,
            true,
            hx_i2c->_addr);

        i2c_slave_init(
            hx_i2c->_i2c,
            hx_i2c->_addr,
            &hx711_i2c_slave_handler);

}

uint8_t hx711_i2c_slave_get_control(
    const hx711_i2c_slave_t* const hx_i2c) {
        assert(hx_i2c != NULL);
        assert(hx_i2c->_memory != NULL);
        return hx_i2c->_memory[HX711_I2C_CONTROL_METADATA_OFFSET_BYTES];
}

void hx711_i2c_slave_set_control(
    hx711_i2c_slave_t* const hx_i2c,
    const uint8_t control) {
        assert(hx_i2c != NULL);
        assert(hx_i2c->_memory != NULL);
        hx_i2c->_memory[HX711_I2C_CONTROL_METADATA_OFFSET_BYTES] = control;
}

void hx711_i2c_slave_control_set_ready_state(
    hx711_i2c_slave_t* const hx_i2c,
    const bool val) {
        assert(hx_i2c != NULL);
        uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        hx711_i2c_control_set_ready_state(val, &ctrl);
        hx711_i2c_slave_set_control(hx_i2c, ctrl);
}

void hx711_i2c_slave_control_set_new_value_state(
    hx711_i2c_slave_t* const hx_i2c,
    const bool is_new) {
        assert(hx_i2c != NULL);
        uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        hx711_i2c_control_set_new_value_state(is_new, &ctrl);
        hx711_i2c_slave_set_control(hx_i2c, ctrl);
}

void hx711_i2c_slave_control_set_power_state(
    hx711_i2c_slave_t* const hx_i2c,
    const bool state) {
        assert(hx_i2c != NULL);
        uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        hx711_i2c_control_set_power_state(state, &ctrl);
        hx711_i2c_slave_set_control(hx_i2c, ctrl);
}

void hx711_i2c_slave_control_set_gain(
    hx711_i2c_slave_t* const hx_i2c,
    const hx711_gain_t gain) {
        assert(hx_i2c != NULL);
        uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        hx711_i2c_control_set_gain(gain, &ctrl);
        hx711_i2c_slave_set_control(hx_i2c, ctrl);
}

void hx711_i2c_slave_control_set_rate(
    hx711_i2c_slave_t* const hx_i2c,
    const hx711_rate_t rate) {
        assert(hx_i2c != NULL);
        uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        hx711_i2c_control_set_rate(rate, &ctrl);
        hx711_i2c_slave_set_control(hx_i2c, ctrl);
}

bool hx711_i2c_slave_control_get_ready_state(
    hx711_i2c_slave_t* const hx_i2c) {
        assert(hx_i2c != NULL);
        const uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        return hx711_i2c_control_get_ready_state(ctrl);
}

bool hx711_i2c_slave_control_get_new_value_state(
    hx711_i2c_slave_t* const hx_i2c) {
        assert(hx_i2c != NULL);
        const uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        return hx711_i2c_control_get_new_value_state(ctrl);
}

bool hx711_i2c_slave_control_get_power_state(
    hx711_i2c_slave_t* const hx_i2c) {
        assert(hx_i2c != NULL);
        const uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        return hx711_i2c_control_get_power_state(ctrl);
}

hx711_gain_t hx711_i2c_slave_control_get_gain(
    hx711_i2c_slave_t* const hx_i2c) {
        assert(hx_i2c != NULL);
        const uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        return (hx711_gain_t)hx711_i2c_control_get_gain(ctrl);
}

hx711_rate_t hx711_i2c_slave_control_get_rate(
    hx711_i2c_slave_t* const hx_i2c) {
        assert(hx_i2c != NULL);
        const uint8_t ctrl = hx711_i2c_slave_get_control(hx_i2c);
        return (hx711_rate_t)hx711_i2c_control_get_rate(ctrl);
}

void hx711_i2c_slave_get_data(
    hx711_i2c_slave_t* const hx_i2c,
    uint8_t* const data) {
        assert(hx_i2c != NULL);
        assert(data != NULL);
        memcpy(data,
            &hx_i2c->_memory[HX711_I2C_CONTROL_DATA_OFFSET_BYTES],
            HX711_I2C_CONTROL_DATA_SIZE_BYTES);
}

void hx711_i2c_slave_set_data(
    hx711_i2c_slave_t* const hx_i2c,
    const uint8_t* const data) {
        assert(hx_i2c != NULL);
        assert(data != NULL);
        memcpy(&hx_i2c->_memory[HX711_I2C_CONTROL_DATA_OFFSET_BYTES],
            data,
            HX711_I2C_CONTROL_DATA_SIZE_BYTES);
}

void hx711_i2c_slave_close(hx711_i2c_slave_t* const hx_i2c) {
    assert(hx_i2c != NULL);
    assert(hx_i2c->_i2c != NULL);
    i2c_slave_deinit(hx_i2c->_i2c);
    i2c_deinit(hx_i2c->_i2c);
    hx711_i2c_slave_remove_slave(hx_i2c);
}

void hx711_i2c_slave_handler(
    i2c_inst_t* i2c,
    i2c_slave_event_t event) {

        assert(i2c != NULL);

        hx711_i2c_slave_t* hx_i2c;

        hx711_i2c_slave_get_slave(i2c, &hx_i2c);

        assert(hx_i2c != NULL);
        assert(i2c == hx_i2c->_i2c);

        switch(event) {
        case I2C_SLAVE_RECEIVE:
            // data available from master to read
            hx_i2c->_indata = i2c_read_byte_raw(hx_i2c->_i2c);
            break;

        case I2C_SLAVE_REQUEST:
            // send data
            i2c_write_raw_blocking(
                i2c,
                (const uint8_t*)hx_i2c->_memory,
                HX711_I2C_CONTROL_TOTAL_BYTES);

            // then update the control
            hx711_i2c_slave_control_set_new_value_state(hx_i2c, false);

            break;

        case I2C_SLAVE_FINISH:
        default:
            // prepare for next transfer
            break;
        }

}

void hx711_i2c_slave_update_loop(
    hx711_i2c_slave_t* const hx_i2c) {

        int32_t val;
        uint8_t valBytes[HX711_I2C_CONTROL_DATA_SIZE_BYTES] = { 0 };
        hx711_i2c_command_t cmd;
        uint8_t data;

        // continuing updating data while this flag is set
        while(hx_i2c->_updating) {

            // only get new hx711 values if the slave is ready and the chip
            // is in a powered-on state
            if(hx711_i2c_slave_control_get_ready_state(hx_i2c) && 
                hx711_i2c_slave_control_get_power_state(hx_i2c)) {
                if(hx711_get_value_noblock(hx_i2c->_hx, &val)) {
                    hx711_i2c_value_to_array(val, valBytes);
                    UTIL_INTERRUPTS_OFF_BLOCK(
                        hx711_i2c_slave_set_data(hx_i2c, valBytes);
                        hx711_i2c_slave_control_set_new_value_state(hx_i2c, true);
                    );
                }
            }

            // make a local copy of the input data, for this iteration
            // and clear it for the next one
            UTIL_INTERRUPTS_OFF_BLOCK(
                data = hx_i2c->_indata;
                hx_i2c->_indata = 0;
            );

            // determine what command has been sent
            cmd = hx711_i2c_command_get_command(data);

            switch(cmd) {
            case hx711_i2c_command_none:
            case hx711_i2c_command_get_value:
            default:
                // do nothing in these cases;
                // slave auto-updates values from hx711
                break;

            case hx711_i2c_command_change_power_state:

                // changing power state either up or down so change
                // control data to indicate the slave is not ready
                // and no new data is available

                hx711_i2c_slave_control_set_ready_state(
                    hx_i2c,
                    false);

                hx711_i2c_slave_control_set_new_value_state(
                    hx_i2c,
                    false);

                if(hx711_i2c_command_get_power_state(data)) {
                    // powering up...

                    hx711_power_up(
                        hx_i2c->_hx,
                        hx711_i2c_command_get_gain(data));

                    hx711_i2c_slave_control_set_power_state(
                        hx_i2c,
                        true);

                    hx711_wait_settle(
                        hx711_i2c_command_get_rate(data));

                }
                else {
                    // powering down...
                    hx711_power_down(hx_i2c->_hx);
                    hx711_i2c_slave_control_set_power_state(hx_i2c, false);
                    hx711_wait_power_down();
                }

                // with the power state change over, change the control
                // data to indicate the slave is ready for new instructions
                hx711_i2c_slave_control_set_ready_state(hx_i2c, true);

                break;

            case hx711_i2c_command_change_gain:

                hx711_i2c_slave_control_set_ready_state(hx_i2c, false);
                hx711_i2c_slave_control_set_new_value_state(hx_i2c, false);

                hx711_set_gain(
                    hx_i2c->_hx,
                    hx711_i2c_command_get_gain(data));

                hx711_wait_settle(
                    hx711_i2c_command_get_rate(data));

                hx711_i2c_slave_control_set_ready_state(hx_i2c, true);

                break;

            }

        }
}
