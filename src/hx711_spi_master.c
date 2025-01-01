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
#include "../include/hx711_spi_master.h"
#include "../include/util.h"

const hx711_spi_frame_t HX711_SPI_NULL_FRAME = {
    .not_null =         false,
    .is_first =         false,
    .is_last =          false,
    .is_continuing =    false,
    .unused_4 =         false,
    .unused_5 =         false,
    .unused_6 =         false,
    .unused_7 =         false,
    .data =             0
};

void hx711_spi_frame_to_buffer(
    const hx711_spi_frame_t* const frame,
    hx711_spi_buffer_t* const buffer) {

        assert(frame != NULL);
        assert(buffer != NULL);
        assert(hx711_spi_buffer_t == uint16_t);

        // avoid repeated ptr deref
        hx711_spi_buffer_t temp = 0;

        temp = util_set_bits16(
            temp,
            HX711_SPI_FRAME_FLAGS_NOT_NULL_OFFSET,
            HX711_SPI_FRAME_FLAGS_NOT_NULL_SIZE,
            frame->not_null);

        temp = util_set_bits16(
            temp,
            HX711_SPI_FRAME_FLAGS_FIRST_OFFSET,
            HX711_SPI_FRAME_FLAGS_FIRST_SIZE,
            frame->is_first);

        temp = util_set_bits16(
            temp,
            HX711_SPI_FRAME_FLAGS_LAST_OFFSET,
            HX711_SPI_FRAME_FLAGS_LAST_SIZE,
            frame->is_last);

        temp = util_set_bits16(
            temp,
            HX711_SPI_FRAME_FLAGS_CONTINUING_OFFSET,
            HX711_SPI_FRAME_FLAGS_CONTINUING_SIZE,
            frame->is_continuing);

        temp = util_set_bits16(
            temp,
            HX711_SPI_FRAME_DATA_OFFSET,
            HX711_SPI_FRAME_DATA_SIZE,
            frame->data);

        *buffer = temp;

}

void hx711_spi_buffer_to_frame(
    const hx711_spi_buffer_t* const buffer,
    hx711_spi_frame_t* const frame) {

        assert(buffer != NULL);
        assert(frame != NULL);
        assert(hx711_spi_buffer_t == uint16_t);

        // avoid repeated ptr deref
        const hx711_spi_buffer_t buff = *buffer;
        hx711_spi_frame_t fr = { 0 };

        fr.not_null = (bool)util_get_bits16(
            buff,
            HX711_SPI_FRAME_FLAGS_NOT_NULL_OFFSET,
            HX711_SPI_FRAME_FLAGS_NOT_NULL_SIZE);

        fr.is_first = (bool)util_get_bits16(
            buff,
            HX711_SPI_FRAME_FLAGS_FIRST_OFFSET,
            HX711_SPI_FRAME_FLAGS_FIRST_SIZE);

        fr.is_last = (bool)util_get_bits16(
            buff,
            HX711_SPI_FRAME_FLAGS_LAST_OFFSET,
            HX711_SPI_FRAME_FLAGS_LAST_SIZE);

        fr.is_continuing = util_get_bits16(
            buff,
            HX711_SPI_FRAME_FLAGS_CONTINUING_OFFSET,
            HX711_SPI_FRAME_FLAGS_CONTINUING_SIZE);

        fr.data = util_get_bits16(
            buff,
            HX711_SPI_FRAME_DATA_OFFSET,
            HX711_SPI_FRAME_DATA_SIZE);

        *frame = fr;

}

size_t hx711_spi_calculate_frame_count(
    const size_t bitsLen) {

        uint32_t rem;
        uint32_t result;

        result = divmod_u32u32_rem(
            bitsLen,
            HX711_SPI_FRAME_DATA_SIZE,
            &rem);

        if(rem > 0) {
            result++;
        }

        return (size_t)result;

}

void hx711_spi_send_data_chunked(
    spi_inst_t* const spi,
    const uint8_t* const data,
    const size_t dataLenBytes) {

        assert(spi != NULL);
        assert(data != NULL);
        assert(dataLenBytes > 0);
        assert(HX711_SPI_BITS_PER_TRANSFER == 16);
        assert(hx711_spi_buffer_t == uint16_t);
        assert(spi_is_writable(spi));

        hx711_spi_frame_t frame;
        hx711_spi_buffer_t outbuffer;
        const size_t numFrames = hx711_spi_calculate_frame_count(
            dataLenBytes * 8);

        for(size_t i = 0; i < numFrames; ++i) {

            // reset memory
            memset(&frame, 0, sizeof(frame));

            frame.not_null = true;

            // flag the first frame
            if(i == 0) {
                frame.is_first = true;
            }

            // flag any frame which isn't the first and isn't the last
            if(i > 0 && i < (numFrames - 1)) {
                frame.is_continuing = true;
            }

            // flag the last frame
            if(i == (numFrames - 1)) {
                frame.is_last = true;
            }

            frame.data = data[i];

            hx711_spi_frame_to_buffer(
                &frame,
                &outbuffer);

            spi_write16_blocking(
                spi,
                &outbuffer,
                1);

        }

}

bool hx711_spi_try_receive_frame(
    spi_inst_t* const spi,
    hx711_spi_frame_t* const frame) {

        assert(spi != NULL);
        assert(frame != NULL);
        assert(HX711_SPI_BITS_PER_TRANSFER == 16);
        assert(hx711_spi_buffer_t == uint16_t);
        assert(spi_is_readable(spi));

        if(!spi_is_readable(spi)) {
            return false;
        }

        hx711_spi_receive_frame_blocking(spi, frame);

        return true;

}

void hx711_spi_receive_frame_blocking(
    spi_inst_t* const spi,
    hx711_spi_frame_t* const frame) {

        assert(spi != NULL);
        assert(frame != NULL);
        assert(HX711_SPI_BITS_PER_TRANSFER == 16);
        assert(hx711_spi_buffer_t == uint16_t);

        hx711_spi_buffer_t inbuffer = 0;
        hx711_spi_buffer_t outbuffer = 0;

        hx711_spi_frame_to_buffer(
            &HX711_SPI_NULL_FRAME,
            &outbuffer);

        // transmit a null frame while receiving
        spi_read16_blocking(
            spi,
            outbuffer,
            &inbuffer,
            1);

        hx711_spi_buffer_to_frame(
            &inbuffer,
            frame);

}

void hx711_spi_receive_first_frame_blocking(
    spi_inst_t* const spi,
    hx711_spi_frame_t* const frame) {

        assert(spi != NULL);
        assert(frame != NULL);

        hx711_spi_frame_t f;

        do {
            hx711_spi_receive_frame_blocking(spi, &f);
        }
        while(!f.is_first);

        *frame = f;

}

bool hx711_spi_try_receive_data(
    spi_inst_t* const spi,
    uint8_t* const data,
    const size_t dataLenBytes) {

        assert(spi != NULL);
        assert(data != NULL);
        assert(dataLenBytes > 0);

        if(!spi_is_readable(spi)) {
            return false;
        }

        size_t receivedFrames = 0;
        hx711_spi_frame_t frame = { 0 };
        const size_t expectedFrames = hx711_spi_calculate_frame_count(
            dataLenBytes * 8);

        hx711_spi_receive_frame_blocking(spi, &frame);

        // failed to sync
        if(!frame.is_first) {
            return false;
        }

        // extract the first frame's data
        data[receivedFrames] = frame.data;
        receivedFrames++;

        // extract the rest of the frames' data
        while(!frame.is_last && receivedFrames <= expectedFrames) {
            hx711_spi_receive_frame_blocking(spi, &frame);
            data[receivedFrames] = frame.data;
            receivedFrames++;
        }

        return true;

}

size_t hx711_spi_receive_data_chunked(
    spi_inst_t* const spi,
    uint8_t* const data,
    const size_t dataLenBytes) {

        assert(spi != NULL);
        assert(data != NULL);
        assert(dataLenBytes > 0);

        hx711_spi_frame_t inframe;
        size_t frameCount = 0;
        const size_t maxFrameCount = hx711_spi_calculate_frame_count(
            dataLenBytes * 8);

        // get (sync) and process first frame
        hx711_spi_receive_first_frame_blocking(
            spi,
            &inframe);

        data[frameCount] = inframe.data;
        frameCount++;

        // now get and process any remaining frames
        while(!inframe.is_last) {

            hx711_spi_receive_frame_blocking(
                spi,
                &inframe);

            data[frameCount] = inframe.data;
            frameCount++;

            // and stop if more frames than expected
            if(frameCount >= maxFrameCount) {
                break;
            }

        }

        return frameCount * 8;

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
            HX711_SPI_BITS_PER_TRANSFER,
            SPI_CPOL_0,
            SPI_CPHA_0,
            SPI_MSB_FIRST);

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

        uint8_t buffer[HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES];

        hx711_remote_request_to_buffer(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            hx711_spi_send_data_chunked(
                hx_spi->_spi,
                buffer,
                HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES);
        );

}

int hx711_spi_master_get_control(
    hx711_spi_master_t* const hx_spi,
    hx711_remote_control_t* const ctrl) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(ctrl != NULL);

        uint8_t buffer[HX711_REMOTE_CONTROL_TOTAL_BYTES];

        size_t bytesRead = 0;

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            bytesRead = hx711_spi_receive_data_chunked(
                hx_spi->_spi,
                buffer,
                HX711_REMOTE_CONTROL_TOTAL_BYTES);
        );

        if(bytesRead != HX711_REMOTE_CONTROL_TOTAL_BYTES) {
            return PICO_ERROR_IO;
        }

        hx711_remote_buffer_to_control(
            buffer,
            ctrl);

        return PICO_OK;

}

int32_t hx711_spi_master_get_value_blocking(
    hx711_spi_master_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        hx711_remote_control_t ctrl = { 0 };

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

        uint8_t buffer[HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES];

        hx711_remote_request_to_buffer(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            hx711_spi_send_data_chunked(
                hx_spi->_spi,
                buffer,
                HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES);
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

        uint8_t buffer[HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES];

        hx711_remote_request_to_buffer(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            hx711_spi_send_data_chunked(
                hx_spi->_spi,
                buffer,
                HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES);
        );

}
