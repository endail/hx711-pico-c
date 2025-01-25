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
#include <hardware/spi.h>
#include <pico/error.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/hx711.h"
#include "../include/hx711_remote.h"
#include "../include/hx711_spi_master.h"
#include "../include/spifixedframe.h"
#include "../include/util.h"

void hx711_spi_serialise_request(
    const hx711_remote_request_t* const req,
    uint8_t* const buffer) {

        // buffer is guaranteed to be of sufficient size by
        // the calling function

        assert(req != NULL);
        assert(buffer != NULL);

        uint8_t* ptr = buffer;
        uint8_t crc;

        // set request data at start of buffer
        hx711_remote_serialise_request(req, ptr);

        // calculate crc based on data currently in buffer
        crc = util_crc8(*ptr, HX711_SPI_CRC8_POLYNOMIAL);

        // now increment the pointer to the next address after
        // request data
        ptr += HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES;

        // and set the crc
        *ptr = crc;

}

void hx711_spi_serialise_control(
    const hx711_remote_control_t* const ctrl,
    uint8_t* const buffer) {

        // buffer is guaranteed to be of sufficient size by
        // the calling function

        assert(ctrl != NULL);
        assert(buffer != NULL);

        uint8_t* ptr = buffer;
        uint32_t crc;

        // set control data at start of buffer
        hx711_remote_serialise_control(ctrl, ptr);

        // calculate crc based on data currently in buffer
        crc = util_crc32(
            ptr,
            HX711_REMOTE_CONTROL_TOTAL_BYTES,
            HX711_SPI_CRC32_POLYNOMIAL);

        // now increment the pointer to the next address after
        // control data
        ptr += HX711_REMOTE_CONTROL_TOTAL_BYTES;

        // and set the crc
        memcpy(ptr, &crc, HX711_SPI_REMOTE_CONTROL_CRC_SIZE_BYTES);

}

bool hx711_spi_deserialise_request(
    const uint8_t* const buffer,
    hx711_remote_request_t* const req) {

        assert(buffer != NULL);
        assert(req != NULL);

        // buffer is guaranteed to be of sufficient size by
        // the calling function

        uint8_t* ptr = (uint8_t*)buffer;
        uint8_t calcd_crc;
        uint8_t raw_crc;

        // parse out the request data
        hx711_remote_deserialise_request(ptr, req);

        // calculate the crc of the request data
        calcd_crc = util_crc8(*ptr, HX711_SPI_CRC8_POLYNOMIAL);

        // increment the pointer to the transmitted crc
        ptr += HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES;
        raw_crc = *ptr;

        // and check if crcs match
        return calcd_crc == raw_crc;

}

bool hx711_spi_deserialise_control(
    const uint8_t* const buffer,
    hx711_remote_control_t* const ctrl) {

        assert(buffer != NULL);
        assert(ctrl != NULL);

        // buffer is guaranteed to be of sufficient size by
        // the calling function

        uint8_t* ptr = (uint8_t*)buffer;
        uint32_t calcd_crc;
        uint32_t raw_crc;

        // parse out the control data
        hx711_remote_deserialise_control(ptr, ctrl);

        // calculate the crc of the control data
        calcd_crc = util_crc32(
            ptr,
            HX711_SPI_REMOTE_CONTROL_CRC_SIZE_BYTES,
            HX711_SPI_CRC32_POLYNOMIAL);

        // increment the pointer to the transmitted crc
        ptr += HX711_REMOTE_CONTROL_TOTAL_BYTES;
        memcpy(&raw_crc, ptr, sizeof(raw_crc));

        // and check if crcs match
        return calcd_crc == raw_crc;

}

void hx711_spi_master_init(
    hx711_spi_master_t* const hx_spi,
    const hx711_spi_master_config_t* const hx_spi_config) {

        assert(hx_spi != NULL);
        assert(hx_spi_config != NULL);

        check_gpio_param(hx_spi_config->sck_pin);
        check_gpio_param(hx_spi_config->csn_pin);
        check_gpio_param(hx_spi_config->rx_pin);
        check_gpio_param(hx_spi_config->tx_pin);

        assert(hx_spi_config->spi != NULL);
        assert(hx_spi_config->baud_rate > 0);

        hx_spi->_sck_pin = hx_spi_config->sck_pin;
        hx_spi->_csn_pin = hx_spi_config->csn_pin;
        hx_spi->_rx_pin = hx_spi_config->rx_pin;
        hx_spi->_tx_pin = hx_spi_config->tx_pin;

        hx_spi->_spi = hx_spi_config->spi;
        hx_spi->_baud_rate = hx_spi_config->baud_rate;

        gpio_init(hx_spi->_sck_pin);
        gpio_init(hx_spi->_csn_pin);
        gpio_init(hx_spi->_rx_pin);
        gpio_init(hx_spi->_tx_pin);

        gpio_set_dir(hx_spi->_sck_pin, true);
        gpio_set_dir(hx_spi->_csn_pin, true);
        gpio_set_dir(hx_spi->_rx_pin, false);
        gpio_set_dir(hx_spi->_tx_pin, true);

        gpio_put(hx_spi->_csn_pin, true);

        gpio_set_function(hx_spi->_sck_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_csn_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_rx_pin, GPIO_FUNC_SPI);
        gpio_set_function(hx_spi->_tx_pin, GPIO_FUNC_SPI);

        spi_init(
            hx_spi->_spi,
            hx_spi->_baud_rate);

        spi_set_format(
            hx_spi->_spi,
            HX711_SPI_SPI_DATA_BITS,
            HX711_SPI_SPI_POLARITY,
            HX711_SPI_SPI_PHASE,
            HX711_SPI_SPI_ORDER);

        spi_set_slave(
            hx_spi->_spi,
            false);

}

void hx711_spi_master_close(
    hx711_spi_master_t* const hx_spi) {
        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        spi_deinit(hx_spi->_spi);
}

void hx711_spi_master_set_gain(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain,
    const hx711_rate_t rate) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(hx711_is_gain_valid(gain));
        assert(hx711_is_rate_valid(rate));

        const hx711_remote_request_t req = {
            .cmd = hx711_remote_command_change_gain,
            .gain = gain,
            .rate = rate
        };

        uint8_t buffer[HX711_SPI_REMOTE_REQUEST_TOTAL_BYTES];

        hx711_spi_serialise_request(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            spifixedframe_send_bytes(
                hx_spi->_spi,
                buffer,
                HX711_SPI_REMOTE_REQUEST_TOTAL_BYTES);
        );

}

int hx711_spi_master_get_control(
    hx711_spi_master_t* const hx_spi,
    hx711_remote_control_t* const ctrl) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(ctrl != NULL);

        uint8_t buffer[HX711_SPI_REMOTE_CONTROL_TOTAL_BYTES];

        bool success = false;

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            success = spifixedframe_recv_bytes(
                hx_spi->_spi,
                buffer,
                HX711_SPI_REMOTE_CONTROL_TOTAL_BYTES);
        );

        UTIL_RETURNIF(!success, PICO_ERROR_IO);

        success = hx711_spi_deserialise_control(
            buffer,
            ctrl);

        return success ? PICO_OK : PICO_ERROR_IO;

}

int32_t hx711_spi_master_get_value_blocking(
    hx711_spi_master_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        hx711_remote_control_t ctrl;

        while(hx711_spi_master_get_control(hx_spi, &ctrl) != PICO_OK) {
            if(!hx711_remote_control_ok(&ctrl)) {
                continue;
            }
        }

        return ctrl.value;

}

void hx711_spi_master_power_up(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain,
    const hx711_rate_t rate) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(hx711_is_gain_valid(gain));
        assert(hx711_is_rate_valid(rate));

        const hx711_remote_request_t req = {
            .cmd = hx711_remote_command_change_power_state,
            .power_state = true,
            .gain = gain,
            .rate = rate
        };

        uint8_t buffer[HX711_SPI_REMOTE_REQUEST_TOTAL_BYTES];

        hx711_spi_serialise_request(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            spifixedframe_send_bytes(
                hx_spi->_spi,
                buffer,
                HX711_SPI_REMOTE_REQUEST_TOTAL_BYTES);
        );

}

void hx711_spi_master_power_down(
    hx711_spi_master_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        const hx711_remote_request_t req = {
            .cmd = hx711_remote_command_change_power_state,
            .power_state = false
        };

        uint8_t buffer[HX711_SPI_REMOTE_REQUEST_TOTAL_BYTES];

        hx711_spi_serialise_request(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            spifixedframe_send_bytes(
                hx_spi->_spi,
                buffer,
                HX711_SPI_REMOTE_REQUEST_TOTAL_BYTES);
        );

}
