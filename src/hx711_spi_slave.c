// MIT License
// 
// Copyright (c) 2023 Daniel Robertson
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
#include <stdbool.h>
#include <stdint.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "../include/hx711.h"
#include "../include/hx711_spi_slave.h"
#include "../include/util.h"

void hx711_spi_slave_init(
    hx711_spi_slave_t* const hx_spi,
    const hx711_spi_slave_config_t * const hx_spi_config) {

        assert(hx_spi != NULL);
        assert(hx_spi_config != NULL);

        check_gpio_param(hx_spi_config->rx_pin);
        check_gpio_param(hx_spi_config->sck_pin);
        check_gpio_param(hx_spi_config->tx_pin);
        check_gpio_param(hx_spi_config->csn_pin);

        assert(hx_spi_config->spi != NULL);
        assert(hx_spi_config->baud_rate > 0);

        hx_spi->_rx_pin = hx_spi_config->rx_pin;
        hx_spi->_sck_pin = hx_spi_config->sck_pin;
        hx_spi->_tx_pin = hx_spi_config->tx_pin;
        hx_spi->_csn_pin = hx_spi_config->csn_pin;

        hx_spi->_spi = hx_spi_config->spi;
        hx_spi->_baud_rate = hx_spi_config->baud_rate;

        gpio_set_function(hx_spi->_rx_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_sck_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_tx_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_csn_pin, GPIO_FUNC_SPI);

        spi_init(hx_spi->_spi, hx_spi->_baud_rate);
        spi_set_slave(hx_spi->_spi, false);

}

void hx711_spi_slave_close(hx711_spi_slave_t* const hx_spi) {
    assert(hx_spi != NULL);
    assert(hx_spi->_spi != NULL);
    spi_deinit(hx_spi->_spi);
}

void hx711_spi_slave_listen(hx711_spi_slave_t* const hx_spi) {

    hx711_spi_command_t cmd;
    uint8_t data;
    hx711_gain_t gain;
    int32_t out;

    while(true) {

        spi_read_blocking(
            hx_spi->_spi,
            hx711_spi_command_none,
            &data,
            sizeof(data));

        cmd = (hx711_spi_command_t)(data & 0b00000111);
        data = data >> 5;

        switch(cmd) {
            default:
            case hx711_spi_command_get_value:
                out = hx711_get_value(hx_spi->_hx);
                spi_write_blocking(hx_spi->_spi, (uint8_t*)&out, sizeof(out));
                break;
            case hx711_spi_command_set_gain:
                gain = hx711_spi_spi_gain_to_gain(data);
                hx711_set_gain(hx_spi->_hx, gain);
                break;
            case hx711_spi_command_power_up:
                gain = hx711_spi_spi_gain_to_gain(data);
                hx711_power_up(hx_spi->_hx, gain);
                break;
            case hx711_spi_command_power_down:
                hx711_power_down(hx_spi->_hx);
                break;
        }

    }

}
