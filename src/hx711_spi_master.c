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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "../include/hx711.h"
#include "../include/hx711_spi_master.h"
#include "../include/util.h"

#include <stdlib.h>
#include <stdio.h>
#include "pico/stdio.h"

void hx711_spi_master_init(
    hx711_spi_master_t* const hx_spi,
    const hx711_spi_master_config_t * const hx_spi_config) {

        assert(hx_spi != NULL);
        assert(hx_spi_config != NULL);

        //check_gpio_param(hx_spi_config->rx_pin);
        //check_gpio_param(hx_spi_config->sck_pin);
        //check_gpio_param(hx_spi_config->tx_pin);
        //check_gpio_param(hx_spi_config->csn_pin);

        assert(hx_spi_config->spi != NULL);
        assert(hx_spi_config->baud_rate > 0);

        //hx_spi->_rx_pin = hx_spi_config->rx_pin;
        //hx_spi->_sck_pin = hx_spi_config->sck_pin;
        //hx_spi->_tx_pin = hx_spi_config->tx_pin;
        //hx_spi->_csn_pin = hx_spi_config->csn_pin;

        hx_spi->_spi = hx_spi_config->spi;
        hx_spi->_baud_rate = hx_spi_config->baud_rate;

        //gpio_set_dir(hx_spi->_rx_pin, false);
        //gpio_set_dir(hx_spi->_sck_pin, true);
        //gpio_set_dir(hx_spi->_tx_pin, true);
        //gpio_set_dir(hx_spi->_csn_pin, true);

        gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
        gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
        gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
        gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);

        //gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
        gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
        gpio_pull_up(PICO_DEFAULT_SPI_CSN_PIN);

        //gpio_pull_up(PICO_DEFAULT_SPI_RX_PIN);

        bi_decl(bi_4pins_with_func(
            PICO_DEFAULT_SPI_RX_PIN,
            PICO_DEFAULT_SPI_TX_PIN,
            PICO_DEFAULT_SPI_SCK_PIN,
            PICO_DEFAULT_SPI_CSN_PIN,
            GPIO_FUNC_SPI));

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

        const uint8_t xfer = hx711_spi_create_xfer(
            hx711_spi_command_set_gain,
            hx711_spi_gain_to_spi_gain(gain));

        HX711_SPI_ATOMIC(hx_spi, 
            spi_write_blocking(
                hx_spi->_spi,
                &xfer,
                sizeof(xfer));
        );

}

bool hx711_spi_master_get_value(
    hx711_spi_master_t* const hx_spi,
    int32_t* const val) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        //uint8_t inbuff[5] = { 0 };
        //uint8_t outbuff[5] = { 0 };
        hx711_spi_frame_t inframe;
        hx711_spi_frame_t outframe;

        memset(&inframe, 0, sizeof(inframe));
        memset(&outframe, 0, sizeof(outframe));

        outframe.command = (uint8_t)hx711_spi_command_get_value;
        outframe.checksum = hx711_spi_generate_checksum(
            (uint8_t*)&outframe, sizeof(outframe) - 1);

        gpio_put(PICO_DEFAULT_SPI_CSN_PIN, GPIO_IN);
        //uint8_t cmd = hx711_spi_command_get_value;
        //spi_write_blocking(
        //    hx_spi->_spi,
        //    &cmd,
        //    1);
        //spi_write_blocking(hx_spi->_spi, &cmd, 1); //dummy
        //spi_read_blocking(
        //    hx_spi->_spi,
        //    0,
        //    (uint8_t*)&val,
        //    4);

        spi_write_blocking(
            hx_spi->_spi,
            (uint8_t*)&outframe,
            sizeof(hx711_spi_frame_t));

        spi_read_blocking(
            hx_spi->_spi,
            0,
            (uint8_t*)&inframe,
            sizeof(hx711_spi_frame_t));

        gpio_put(PICO_DEFAULT_SPI_CSN_PIN, true);
        //sleep_ms(10);

        const bool check = inframe.checksum == hx711_spi_generate_checksum(
            (uint8_t*)&inframe, sizeof(inframe) - 1);

        //printf("Sent: %i %i %i %i %i\n", outframe.command, outframe.data[0], outframe.data[1], outframe.data[2], outframe.checksum);
        //printf("Recd: %i %i %i %i %i\n", inframe.command, inframe.data[0], inframe.data[1], inframe.data[2], inframe.checksum);
        //printf("Checksum OK?: %s\n", check ? "Yes" : "No");

        if(check) {
            *val = hx711_spi_array_to_value(inframe.data);
        }

        return check;
        //printf("Received value: %li\n", val);
        //printf("======\n");

}

void hx711_spi_master_power_up(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(hx711_is_gain_valid(gain));

        const uint8_t xfer = hx711_spi_create_xfer(
            hx711_spi_command_power_up,
            hx711_spi_gain_to_spi_gain(gain));

        HX711_SPI_ATOMIC(hx_spi, 
            spi_write_read_blocking(
                hx_spi->_spi,
                &xfer,
                NULL,
                sizeof(xfer));
        );

}

void hx711_spi_master_power_down(
    hx711_spi_master_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        const uint8_t xfer = hx711_spi_create_xfer(
            hx711_spi_command_power_down, 0);

        HX711_SPI_ATOMIC(hx_spi, 
            spi_write_blocking(
                hx_spi->_spi,
                &xfer,
                sizeof(xfer));
        );

}

uint8_t hx711_spi_generate_checksum(
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

void hx711_spi_value_to_array(
    const int32_t val,
    uint8_t* const arr) {
        const int32_t extval = (val << 8) >> 8;
        arr[0] = (uint8_t)extval;
        arr[1] = (uint8_t)(extval >> 8);
        arr[2] = (uint8_t)(extval >> 16);
}

int32_t hx711_spi_array_to_value(
    const uint8_t* const arr) {

        int32_t val = 
            (arr[0] << 0) |
            (arr[1] << 8) |
            (arr[2] << 16);

        val = (val << 8) >> 8;

        return val;

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

uint8_t hx711_spi_create_xfer(
    const hx711_spi_command_t cmd,
    const uint8_t data) {
        uint8_t xfer = 0;
        xfer = hx711_spi_put_command_in_xfer(cmd, xfer);
        xfer = hx711_spi_put_data_in_xfer(data, xfer);
        return xfer;
}

void hx711_spi_parse_xfer(
    const uint8_t inbyte,
    hx711_spi_command_t* const cmd,
    uint8_t* const data) {
    
        assert(cmd != NULL);

        *cmd = hx711_spi_get_command_from_xfer(inbyte);
        assert(hx711_spi_is_command_valid(*cmd));

        if(data != NULL) {
            *data = hx711_spi_get_data_from_xfer(inbyte);
        }

}

bool hx711_spi_is_command_valid(
    const hx711_spi_command_t cmd) {
        return (uint8_t)cmd <= hx711_spi_command_get_value;
}

hx711_spi_command_t hx711_spi_get_command_from_xfer(
    const uint8_t data) {
        const hx711_spi_command_t cmd = 
            (hx711_spi_command_t)(data & ((1 << HX711_SPI_COMMAND_BITS) - 1));
        assert(hx711_spi_is_command_valid(cmd));
        return cmd;
}

uint8_t hx711_spi_put_command_in_xfer(
    const hx711_spi_command_t cmd,
    const uint8_t xfer) {
        assert(hx711_spi_is_command_valid(cmd));
        const uint8_t mask = (1 << HX711_SPI_COMMAND_BITS) - 1;
        const uint8_t cmdbits = ((uint8_t)cmd) & mask;
        return xfer | cmdbits;
}

uint8_t hx711_spi_get_data_from_xfer(
    const uint8_t xfer) {
        return xfer >> HX711_SPI_COMMAND_BITS;
}

uint8_t hx711_spi_put_data_in_xfer(
    const uint8_t data,
    const uint8_t xfer) {
        return xfer | (data << HX711_SPI_COMMAND_BITS);
}
