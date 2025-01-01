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
#include <hardware/spi.h>
#include <pico/error.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/hx711.h"
#include "../include/hx711_remote.h"
#include "../include/hx711_spi_slave.h"
#include "../include/util.h"

bool hx711_spi_slave_try_get_request(
    hx711_spi_slave_t* const hx_spi,
    hx711_remote_request_t* const req) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(req != NULL);

        uint8_t data[HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES];

        const bool success = hx711_spi_try_receive_data(
            hx_spi->_spi,
            data,
            HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES);

        if(success) {
            hx711_remote_buffer_to_request(data, req);
        }

        return success;

}

void hx711_spi_slave_transmit_control(
    hx711_spi_slave_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        uint8_t buffer[HX711_REMOTE_CONTROL_TOTAL_BYTES];

        hx711_remote_control_to_buffer(
            &hx_spi->_memory,
            buffer);

        hx711_spi_send_data_chunked(
            hx_spi->_spi,
            buffer,
            HX711_REMOTE_CONTROL_TOTAL_BYTES);

}

void hx711_spi_slave_change_power(
    hx711_spi_slave_t* const hx_spi,
    const hx711_remote_request_t* const req) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(req != NULL);

        hx_spi->_memory.ready_state = false;
        hx_spi->_memory.new_value_state = false;

        if(req->power_state) {
            hx711_power_up(hx_spi->_hx, req->gain);
            hx_spi->_memory.power_state = true;
            hx711_wait_settle(req->rate);
        }
        else {
            hx711_power_down(hx_spi->_hx);
            hx_spi->_memory.power_state = false;
            hx711_wait_power_down();
        }

        hx_spi->_memory.ready_state = true;

}

static void hx711_spi_slave_change_gain(
    hx711_spi_slave_t* const hx_spi,
    const hx711_remote_request_t* const req) {

        assert(hx_spi != NULL);
        assert(req != NULL);

        hx_spi->_memory.ready_state = false;
        hx_spi->_memory.new_value_state = false;

        hx711_set_gain(hx_spi->_hx, req->gain);
        hx711_wait_settle(req->rate);

        hx_spi->_memory.ready_state = true;

}

void hx711_spi_slave_init(
    hx711_spi_slave_t* const hx_spi,
    const hx711_spi_slave_config_t * const hx_spi_config) {

        assert(hx_spi != NULL);
        assert(hx_spi_config != NULL);

        check_gpio_param(hx_spi_config->sck_pin);
        check_gpio_param(hx_spi_config->csn_pin);
        check_gpio_param(hx_spi_config->rx_pin);
        check_gpio_param(hx_spi_config->tx_pin);

        assert(hx_spi_config->spi != NULL);
        assert(hx_spi_config->baud_rate > 0);
        assert(hx_spi_config->hx != NULL);

        hx_spi->_hx = hx_spi_config->hx;

        hx_spi->_sck_pin = hx_spi_config->sck_pin;
        hx_spi->_csn_pin = hx_spi_config->csn_pin;
        hx_spi->_rx_pin = hx_spi_config->rx_pin;
        hx_spi->_tx_pin = hx_spi_config->tx_pin;

        hx_spi->_spi = hx_spi_config->spi;
        hx_spi->_baud_rate = hx_spi_config->baud_rate;

        gpio_set_dir(hx_spi->_sck_pin, false);
        gpio_set_dir(hx_spi->_csn_pin, false);
        gpio_set_dir(hx_spi->_rx_pin, false);
        gpio_set_dir(hx_spi->_tx_pin, true);

        gpio_set_function(hx_spi->_sck_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_csn_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_rx_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_tx_pin, GPIO_FUNC_SPI);

        gpio_pull_up(hx_spi->_csn_pin);

        hx_spi->_updating = true;

        spi_init(
            hx_spi->_spi,
            hx_spi->_baud_rate);

        spi_set_format(
            hx_spi->_spi,
            HX711_SPI_BITS_PER_TRANSFER,
            SPI_CPOL_0,
            SPI_CPHA_0,
            SPI_MSB_FIRST);

        spi_set_slave(
            hx_spi->_spi,
            true);

}

void hx711_spi_slave_close(
    hx711_spi_slave_t* const hx_spi) {
        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        spi_deinit(hx_spi->_spi);
}

void hx711_spi_slave_listen(
    hx711_spi_slave_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        int32_t val;
        bool haveRequest = false;
        hx711_remote_request_t req;

        while(hx_spi->_updating) {

            if(hx711_remote_control_ok(&hx_spi->_memory)) {
                if(hx711_get_value_noblock(hx_spi->_hx, &val)) {
                    hx_spi->_memory.value = val;
                    hx_spi->_memory.new_value_state = true;
                    hx711_spi_slave_transmit_control(hx_spi);
                    hx_spi->_memory.new_value_state = false;
                }
            }

            memset(&req, 0, sizeof(req));
            haveRequest = hx711_spi_slave_try_get_request(hx_spi, &req);

            if(haveRequest) {
                switch(req.cmd) {
                case hx711_remote_command_none:
                default:
                    break;

                case hx711_remote_command_get_value:
                    hx711_spi_slave_transmit_control(hx_spi);
                    break;

                case hx711_remote_command_change_power_state:
                    hx711_spi_slave_change_power(hx_spi, &req);
                    break;

                case hx711_remote_command_change_gain:
                hx711_spi_slave_change_gain(hx_spi, &req);
                    break;
                }
            }

        }

}
