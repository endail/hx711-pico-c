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
#include "../include/hx711_spi_master.h"
#include "../include/util.h"

void hx711_spi_master_init(
    hx711_spi_master_t* const hx_spi,
    const hx711_spi_master_config_t * const hx_spi_config) {

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

void hx711_spi_master_close(
    hx711_spi_master_t* const hx_spi) {
        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        spi_deinit(hx_spi->_spi);
}

void hx711_spi_master_set_gain(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(hx711_is_gain_valid(gain));

        const uint8_t data = 
            (hx711_spi_gain_to_spi_gain(gain) << 3) |
            (uint8_t)hx711_spi_command_set_gain;

        spi_write_blocking(
            hx_spi->_spi,
            &data,
            sizeof(data));

}

int32_t hx711_spi_master_get_value(
    hx711_spi_master_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        int32_t val;

        spi_read_blocking(
            hx_spi->_spi,
            hx711_spi_command_none,
            (uint8_t*)&val,
            sizeof(val));

        return val;

}

void hx711_spi_master_power_up(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(hx711_is_gain_valid(gain));

        const uint8_t data = 
            (hx711_spi_gain_to_spi_gain(gain) << 3) |
            (uint8_t)hx711_spi_command_set_gain;

        spi_write_read_blocking(
            hx_spi->_spi,
            &data,
            NULL,
            sizeof(data));

}

void hx711_spi_master_power_down(
    hx711_spi_master_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        const uint8_t cmd = (uint8_t)hx711_spi_command_power_down;

        spi_write_blocking(
            hx_spi->_spi,
            &cmd,
            sizeof(cmd));

}

bool hx711_spi_is_spi_gain_valid(const uint8_t spi_gain) {
    return spi_gain <= count_of(HX711_CLOCK_PULSES) - 1;
}

uint8_t hx711_spi_gain_to_spi_gain(const hx711_gain_t gain) {
    assert(hx711_is_gain_valid(gain));
    return (uint8_t)gain - HX711_READ_BITS;
}

hx711_gain_t hx711_spi_spi_gain_to_gain(const uint8_t spi_gain) {
    assert(hx711_spi_is_spi_gain_valid(spi_gain));
    const hx711_gain_t gain = (hx711_gain_t)HX711_CLOCK_PULSES[spi_gain];
    assert(hx711_is_gain_valid(gain));
    return gain;
}
