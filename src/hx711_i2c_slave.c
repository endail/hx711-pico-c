// MIT License
// 
// Copyright (c) 2025 Daniel Robertson
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
#include <pico/mutex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/hx711.h"
#include "../include/hx711_remote.h"
#include "../include/hx711_i2c_slave.h"
#include "../include/util.h"

hx711_i2c_slave_t* hx711_i2c__slave_map[] = {
    NULL, //...
};

auto_init_mutex(hx711_i2c__slave_mutex);

bool hx711_i2c__slave_add_slave(
    hx711_i2c_slave_t* const slave) {

        assert(slave != NULL);
        assert(HX711_I2C_SLAVE_MAP_SIZE > 0);
        assert(hx711_i2c__slave_map != NULL);

        bool success = false;

        mutex_enter_blocking(&hx711_i2c__slave_mutex);

        for(size_t i = 0; i < HX711_I2C_SLAVE_MAP_SIZE; ++i) {
            if(hx711_i2c__slave_map[i] == NULL) {
                hx711_i2c__slave_map[i] = slave;
                success = true;
                break;
            }
        }

        mutex_exit(&hx711_i2c__slave_mutex);

        return success;

}

void hx711_i2c__slave_remove_slave(
    const hx711_i2c_slave_t* const slave) {

        assert(slave != NULL);
        assert(HX711_I2C_SLAVE_MAP_SIZE > 0);
        assert(hx711_i2c__slave_map != NULL);

        mutex_enter_blocking(&hx711_i2c__slave_mutex);

        for(size_t i = 0; i < HX711_I2C_SLAVE_MAP_SIZE; ++i) {
            if(hx711_i2c__slave_map[i] == slave) {
                hx711_i2c__slave_map[i] = NULL;
                break;
            }
        }

        mutex_exit(&hx711_i2c__slave_mutex);

}

bool hx711_i2c__slave_get_slave(
    const i2c_inst_t* const i2c,
    hx711_i2c_slave_t** slave) {

        assert(i2c != NULL);
        assert(slave != NULL);
        assert(*slave != NULL);
        assert(HX711_I2C_SLAVE_MAP_SIZE > 0);
        assert(hx711_i2c__slave_map != NULL);

        bool success = false;

        mutex_enter_blocking(&hx711_i2c__slave_mutex);

        for(size_t i = 0; i < HX711_I2C_SLAVE_MAP_SIZE; ++i) {
            if(hx711_i2c__slave_map[i]->_i2c == i2c) {
                *slave = hx711_i2c__slave_map[i];
                success = true;
                break;
            }
        }

        mutex_exit(&hx711_i2c__slave_mutex);

        return success;

}

void hx711_i2c_slave_init(
    hx711_i2c_slave_t* const hx_i2c,
    const hx711_i2c_slave_config_t* const hx_i2c_config) {

        assert(hx_i2c != NULL);
        assert(hx_i2c_config != NULL);

        assert(hx_i2c_config->i2c != NULL);
        assert(hx_i2c_config->baud_rate > 0);
        assert(hx_i2c_config->hx != NULL);

        mutex_enter_blocking(&hx711_i2c__slave_mutex);

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

        memset(&hx_i2c->_memory, 0, sizeof(hx_i2c->_memory));
        memset(&hx_i2c->_inreq, 0, sizeof(hx_i2c->_inreq));

        UTIL_INTERRUPTS_OFF_BLOCK(
            hx711_i2c__slave_add_slave(hx_i2c);
        );

        hx711_remote_control_get_defaults(&hx_i2c->_memory);

        hx_i2c->_updating = true;

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

void hx711_i2c_slave_close(
    hx711_i2c_slave_t* const hx_i2c) {
        assert(hx_i2c != NULL);
        assert(hx_i2c->_i2c != NULL);
        UTIL_INTERRUPTS_OFF_BLOCK(
            i2c_slave_deinit(hx_i2c->_i2c);
            i2c_deinit(hx_i2c->_i2c);
            hx711_i2c__slave_remove_slave(hx_i2c);
        );
}

void hx711_i2c_slave_handler(
    i2c_inst_t* i2c,
    i2c_slave_event_t event) {

        assert(i2c != NULL);

        hx711_i2c_slave_t* hx_i2c;
        hx711_i2c__slave_get_slave(i2c, &hx_i2c);

        assert(hx_i2c != NULL);
        assert(i2c == hx_i2c->_i2c);

        switch(event) {
        case I2C_SLAVE_RECEIVE:
            // data available from master to read

            uint8_t reqbuff[HX711_I2C_REMOTE_REQUEST_TOTAL_BYTES];

            for(size_t i = 0; i < HX711_I2C_REMOTE_REQUEST_TOTAL_BYTES; ++i) {
                reqbuff[i] = i2c_read_byte_raw(hx_i2c->_i2c);
            }

            // if crc fails, clear request
            if(!hx711_i2c_deserialise_request(
                reqbuff,
                &hx_i2c->_inreq)) {
                    memset(&hx_i2c->_inreq, 0, sizeof(hx_i2c->_inreq));
            }

            break;

        case I2C_SLAVE_REQUEST:
            // send data

            uint8_t ctrlbuff[HX711_I2C_REMOTE_CONTROL_TOTAL_BYTES];

            hx711_i2c_serialise_control(
                &hx_i2c->_memory,
                ctrlbuff);

            for(size_t i = 0; i < HX711_I2C_REMOTE_CONTROL_TOTAL_BYTES; ++i) {
                i2c_write_byte_raw(i2c, ctrlbuff[i]);
            }

            hx_i2c->_memory.new_value_state = false;

            break;

        case I2C_SLAVE_FINISH:
        default:
            // prepare for next transfer
            // nothing to do so just ignore
            break;
        }

}

void hx711_i2c_slave_update_loop(
    hx711_i2c_slave_t* const hx_i2c) {

        int32_t val;
        hx711_remote_request_t req;

        // continuing updating data while this flag is set
        while(hx_i2c->_updating) {

            // only get new hx711 values if the slave is ready and the chip
            // is in a powered-on state
            if(hx711_remote_control_ok(&hx_i2c->_memory)) {
                if(hx711_get_value_noblock(hx_i2c->_hx, &val)) {
                    UTIL_INTERRUPTS_OFF_BLOCK(
                        hx_i2c->_memory.value = val;
                        hx_i2c->_memory.new_value_state = true;
                    );
                }
            }

            // make a local copy of the input data, for this iteration
            // and clear it for the next one
            UTIL_INTERRUPTS_OFF_BLOCK(
                memcpy(&req, &hx_i2c->_inreq, sizeof(req));
                memset(&hx_i2c->_inreq, 0, sizeof(hx_i2c->_inreq));
            );

            switch(req.cmd) {
            case hx711_remote_command_none:
            case hx711_remote_command_get_value:
            default:
                // do nothing in these cases;
                // slave auto-updates values from hx711
                break;

            case hx711_remote_command_change_power_state:

                // changing power state either up or down so change
                // control data to indicate the slave is not ready
                // and no new data is available

                hx_i2c->_memory.ready_state = false;
                hx_i2c->_memory.new_value_state = false;

                if(req.power_state) {
                    hx711_power_up(hx_i2c->_hx, req.gain);
                    hx_i2c->_memory.power_state = true;
                    hx711_wait_settle(req.rate);
                }
                else {
                    hx711_power_down(hx_i2c->_hx);
                    hx_i2c->_memory.power_state = false;
                    hx711_wait_power_down();
                }

                // with the power state change over, change the control
                // data to indicate the slave is ready for new instructions
                hx_i2c->_memory.ready_state = true;
                break;

            case hx711_remote_command_change_gain:

                hx_i2c->_memory.ready_state = false;
                hx_i2c->_memory.new_value_state = false;

                hx711_set_gain(hx_i2c->_hx, req.gain);
                hx711_wait_settle(req.rate);

                hx_i2c->_memory.ready_state = true;
                break;

            }

        }
}
