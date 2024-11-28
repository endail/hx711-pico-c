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
#include <string.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "../include/hx711.h"
#include "../include/hx711_spi_slave.h"
#include "../include/util.h"

#include <stdlib.h>
#include <stdio.h>
#include "pico/stdio.h"
#include "tusb.h"

void hx711_spi_slave_init(
    hx711_spi_slave_t* const hx_spi,
    const hx711_spi_slave_config_t * const hx_spi_config) {

        assert(hx_spi != NULL);
        assert(hx_spi_config != NULL);

        //check_gpio_param(hx_spi_config->rx_pin);
        //check_gpio_param(hx_spi_config->sck_pin);
        //check_gpio_param(hx_spi_config->tx_pin);
        //check_gpio_param(hx_spi_config->csn_pin);

        assert(hx_spi_config->spi != NULL);
        assert(hx_spi_config->baud_rate > 0);
        assert(hx_spi_config->hx != NULL);

        hx_spi->_hx = hx_spi_config->hx;

        //hx_spi->_rx_pin = hx_spi_config->rx_pin;
        //hx_spi->_sck_pin = hx_spi_config->sck_pin;
        //hx_spi->_tx_pin = hx_spi_config->tx_pin;
        //hx_spi->_csn_pin = hx_spi_config->csn_pin;

        hx_spi->_spi = hx_spi_config->spi;
        hx_spi->_baud_rate = hx_spi_config->baud_rate;

        //gpio_set_dir(hx_spi->_rx_pin, false);
        //gpio_set_dir(hx_spi->_sck_pin, false);
        //gpio_set_dir(hx_spi->_tx_pin, true);
        //gpio_set_dir(hx_spi->_csn_pin, false);

        gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
        gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
        gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
        gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);
        
        gpio_pull_up(PICO_DEFAULT_SPI_CSN_PIN);

        bi_decl(bi_4pins_with_func(
            PICO_DEFAULT_SPI_RX_PIN,
            PICO_DEFAULT_SPI_TX_PIN,
            PICO_DEFAULT_SPI_SCK_PIN,
            PICO_DEFAULT_SPI_CSN_PIN,
            GPIO_FUNC_SPI));

        spi_init(hx_spi->_spi, hx_spi->_baud_rate);
        spi_set_slave(hx_spi->_spi, true);

}

void hx711_spi_slave_close(hx711_spi_slave_t* const hx_spi) {
    assert(hx_spi != NULL);
    assert(hx_spi->_spi != NULL);
    spi_deinit(hx_spi->_spi);
}

void hx711_spi_slave_listen(hx711_spi_slave_t* const hx_spi) {

    uint8_t xfer;
    hx711_spi_command_t cmd;
    uint8_t data;
    hx711_gain_t gain;
    int32_t val;
    //uint8_t inbuff[5] = { 0 };
    //uint8_t outbuff[5] = { 0 };
    bool check;
    hx711_spi_frame_t inframe;
    hx711_spi_frame_t outframe;

    stdio_init_all();

    while (!tud_cdc_connected()) {
        sleep_ms(1);
    }

    while(true) {

        val = hx711_get_value(hx_spi->_hx);

        if(spi_is_readable(hx_spi->_spi)) {

            memset(&inframe, 0, sizeof(inframe));

            spi_read_blocking(
                hx_spi->_spi,
                0,
                (uint8_t*)&inframe,
                sizeof(inframe));

            check = inframe.checksum == hx711_spi_generate_checksum(
                (uint8_t*)&inframe, sizeof(inframe) - 1);

            printf("Recd: %i %i %i %i %i\n", inframe.command, inframe.data[0], inframe.data[1], inframe.data[2], inframe.checksum);
            printf("Checksum OK?: %s\n", check ? "Yes" : "No");

            outframe.command = (uint8_t)hx711_spi_command_get_value;
            hx711_spi_value_to_array(val, outframe.data);
            outframe.checksum = hx711_spi_generate_checksum(
                (uint8_t*)&outframe, sizeof(outframe) - 1);

            spi_write_blocking(
                hx_spi->_spi,
                (uint8_t*)&outframe,
                sizeof(outframe));
        
            printf("Sent: %i %i %i %i %i\n", outframe.command, outframe.data[0], outframe.data[1], outframe.data[2], outframe.checksum);
            printf("Sending value: %li\n", val);
            printf("======\n");


        }

    }



    while(true) {

        // read one byte containing command
        spi_read_blocking(
            hx_spi->_spi,
            3,
            &xfer,
            1);

        // parse the xfer into cmd and data
        hx711_spi_parse_xfer(
            xfer,
            &cmd,
            &data);

        switch(cmd) {

            default:
            case hx711_spi_command_get_value:
                //val = hx711_get_value(hx_spi->_hx);
                printf("Sending value: %li\n", val);
                spi_write_blocking(
                    hx_spi->_spi,
                    NULL,
                    4);
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
