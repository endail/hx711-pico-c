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
#include "../include/hx711_spi_master.h"
#include "../include/util.h"

const hx711_spi_frame_t HX711_SPI_NULL_FRAME = {
    .not_null = false,
    .is_first = false,
    .is_last = false,
    .is_continuing = false,
    .unused_4 = false,
    .unused_5 = false,
    .unused_6 = false,
    .unused_7 = false,
    .data = 0
};

void hx711_spi_frame_to_buffer(
    const hx711_spi_frame_t* const frame,
    uint16_t* const buffer) {

        assert(frame != NULL);
        assert(buffer != NULL);

        *buffer = util_set_bits16(
            *buffer,
            HX711_SPI_FRAME_FLAGS_NOT_NULL_OFFSET,
            HX711_SPI_FRAME_FLAGS_NOT_NULL_SIZE,
            frame->not_null);

        *buffer = util_set_bits16(
            *buffer,
            HX711_SPI_FRAME_FLAGS_FIRST_OFFSET,
            HX711_SPI_FRAME_FLAGS_FIRST_SIZE,
            frame->is_first);

        *buffer = util_set_bits16(
            *buffer,
            HX711_SPI_FRAME_FLAGS_LAST_OFFSET,
            HX711_SPI_FRAME_FLAGS_LAST_SIZE,
            frame->is_last);

        *buffer = util_set_bits16(
            *buffer,
            HX711_SPI_FRAME_FLAGS_CONTINUING_OFFSET,
            HX711_SPI_FRAME_FLAGS_CONTINUING_SIZE,
            frame->is_continuing);

        *buffer = util_set_bits16(
            *buffer,
            HX711_SPI_FRAME_DATA_OFFSET,
            HX711_SPI_FRAME_DATA_SIZE,
            frame->data);

}

void hx711_spi_buffer_to_frame(
    const uint16_t* const buffer,
    hx711_spi_frame_t* const frame) {

        assert(buffer != NULL);
        assert(frame != NULL);

        frame->not_null = (bool)util_get_bits16(
            *buffer,
            HX711_SPI_FRAME_FLAGS_NOT_NULL_OFFSET,
            HX711_SPI_FRAME_FLAGS_NOT_NULL_SIZE);

        frame->is_first = (bool)util_get_bits16(
            *buffer,
            HX711_SPI_FRAME_FLAGS_FIRST_OFFSET,
            HX711_SPI_FRAME_FLAGS_FIRST_SIZE);

        frame->is_last = (bool)util_get_bits16(
            *buffer,
            HX711_SPI_FRAME_FLAGS_LAST_OFFSET,
            HX711_SPI_FRAME_FLAGS_LAST_SIZE);

        frame->is_continuing = util_get_bits16(
            *buffer,
            HX711_SPI_FRAME_FLAGS_CONTINUING_OFFSET,
            HX711_SPI_FRAME_FLAGS_CONTINUING_SIZE);

        frame->data = util_get_bits16(
            *buffer,
            HX711_SPI_FRAME_DATA_OFFSET,
            HX711_SPI_FRAME_DATA_SIZE);

}

void hx711_spi_control_to_buffer(
    const hx711_spi_control_t* const ctrl,
    uint8_t* const buffer) {

        assert(ctrl != NULL);
        assert(buffer != NULL);

        memset(buffer,
            0,
            HX711_SPI_CONTROL_TOTAL_BYTES);

        buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_READY_STATE_OFFSET,
            HX711_SPI_CONTROL_READY_STATE_SIZE,
            (uint8_t)ctrl->ready_state);

        buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_NEW_VALUE_STATE_OFFSET,
            HX711_SPI_CONTROL_NEW_VALUE_STATE_SIZE,
            (uint8_t)ctrl->new_value_state);

        buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_POWER_STATE_OFFSET,
            HX711_SPI_CONTROL_POWER_STATE_SIZE,
            (uint8_t)ctrl->power_state);

        buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_GAIN_OFFSET,
            HX711_SPI_CONTROL_GAIN_SIZE,
            (uint8_t)ctrl->gain);

        buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_RATE_OFFSET,
            HX711_SPI_CONTROL_RATE_SIZE,
            (uint8_t)ctrl->rate);

        hx711_spi_value_to_array(
            ctrl->value,
            &buffer[HX711_SPI_CONTROL_DATA_OFFSET_BYTES]);

}

void hx711_spi_buffer_to_control(
    const uint8_t* const buffer,
    hx711_spi_control_t* const ctrl) {

        assert(buffer != NULL);
        assert(ctrl != NULL);

        ctrl->ready_state = (bool)util_get_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_READY_STATE_OFFSET,
            HX711_SPI_CONTROL_READY_STATE_SIZE);

        ctrl->new_value_state = (bool)util_get_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_NEW_VALUE_STATE_OFFSET,
            HX711_SPI_CONTROL_NEW_VALUE_STATE_SIZE);

        ctrl->power_state = (bool)util_get_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_POWER_STATE_OFFSET,
            HX711_SPI_CONTROL_POWER_STATE_SIZE);

        ctrl->gain = (hx711_gain_t)util_get_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_GAIN_OFFSET,
            HX711_SPI_CONTROL_GAIN_SIZE);

        assert(hx711_is_gain_valid(ctrl->gain));

        ctrl->rate = (hx711_rate_t)util_get_bits8(
            buffer[HX711_SPI_CONTROL_METADATA_OFFSET_BYTES],
            HX711_SPI_CONTROL_RATE_OFFSET,
            HX711_SPI_CONTROL_RATE_SIZE);

        assert(hx711_is_rate_valid(ctrl->rate));

        ctrl->value = hx711_spi_array_to_value(
            &buffer[HX711_SPI_CONTROL_DATA_OFFSET_BYTES]);

        assert(hx711_is_value_valid(ctrl->value));

}

void hx711_spi_request_to_buffer(
    const hx711_spi_request_t* const req,
    uint8_t* const buffer) {

        assert(req != NULL);
        assert(buffer != NULL);

        memset(buffer,
            0,
            HX711_SPI_REQUEST_TOTAL_SIZE_BYTES);

        buffer[HX711_SPI_REQUEST_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_REQUEST_OFFSET_BYTES],
            HX711_SPI_REQUEST_COMMAND_OFFSET,
            HX711_SPI_REQUEST_COMMAND_SIZE,
            (uint8_t)req->cmd);

        buffer[HX711_SPI_REQUEST_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_REQUEST_OFFSET_BYTES],
            HX711_SPI_REQUEST_POWER_STATE_OFFSET,
            HX711_SPI_REQUEST_POWER_STATE_SIZE,
            (uint8_t)req->power_state);

        buffer[HX711_SPI_REQUEST_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_REQUEST_OFFSET_BYTES],
            HX711_SPI_REQUEST_GAIN_OFFSET,
            HX711_SPI_REQUEST_GAIN_SIZE,
            (uint8_t)req->gain);

        buffer[HX711_SPI_REQUEST_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_SPI_REQUEST_OFFSET_BYTES],
            HX711_SPI_REQUEST_RATE_OFFSET,
            HX711_SPI_REQUEST_RATE_SIZE,
            (uint8_t)req->rate);

}

void hx711_spi_buffer_to_request(
    const uint8_t* const buffer,
    hx711_spi_request_t* const req) {

        assert(buffer != NULL);
        assert(req != NULL);

        req->cmd = (hx711_spi_command_t)util_get_bits8(
            buffer[HX711_SPI_REQUEST_OFFSET_BYTES],
            HX711_SPI_REQUEST_COMMAND_OFFSET,
            HX711_SPI_REQUEST_COMMAND_SIZE);

        assert(hx711_spi_command_is_valid(req->cmd));

        req->power_state = (bool)util_get_bits8(
            buffer[HX711_SPI_REQUEST_OFFSET_BYTES],
            HX711_SPI_REQUEST_POWER_STATE_OFFSET,
            HX711_SPI_REQUEST_POWER_STATE_SIZE);

        req->gain = (hx711_gain_t)util_get_bits8(
            buffer[HX711_SPI_REQUEST_OFFSET_BYTES],
            HX711_SPI_REQUEST_GAIN_OFFSET,
            HX711_SPI_REQUEST_GAIN_SIZE);

        assert(hx711_is_gain_valid(req->gain));

        req->rate = (hx711_rate_t)util_get_bits8(
            buffer[HX711_SPI_REQUEST_OFFSET_BYTES],
            HX711_SPI_REQUEST_RATE_OFFSET,
            HX711_SPI_REQUEST_RATE_SIZE);

        assert(hx711_is_rate_valid(req->rate));

}

/*
uint16_t hx711_spi_get_next_chunk(
    const uint8_t* const data,
    size_t* const data_index,
    const size_t data_len,
    uint16_t* const current_chunk,
    size_t* const bits_in_current_chunk) {

        assert(data != NULL);
        assert(data_index != NULL);
        assert(data_len > 0);
        assert(current_chunk != NULL);
        assert(bits_in_current_chunk != NULL);

        uint16_t result = 0;
        size_t bytes_to_process = (*bits_in_current_chunk < HX711_SPI_FRAME_DATA_SIZE) ?
            (data_len - *data_index) : 0;

        for(size_t i = 0; i < bytes_to_process && *bits_in_current_chunk < HX711_SPI_FRAME_DATA_SIZE; i++) {
            uint8_t byte = data[*data_index];
            for(int j = 7; j >= 0 && *bits_in_current_chunk < HX711_SPI_FRAME_DATA_SIZE; j--) {
                *current_chunk <<= 1;
                *current_chunk |= (byte >> j) & 1;
                (*bits_in_current_chunk)++;
            }
            (*data_index)++;
        }

        if(*bits_in_current_chunk >= HX711_SPI_FRAME_DATA_SIZE) {
            //result = *current_chunk & 0x3fff; //14 bits
            result = *current_chunk & ((1 << HX711_SPI_FRAME_DATA_SIZE) - 1);
            *current_chunk >>= HX711_SPI_FRAME_DATA_SIZE;
            *bits_in_current_chunk -= HX711_SPI_FRAME_DATA_SIZE;
        }

        return result;

}

void hx711_spi_combine_chunks(
    uint16_t chunk,
    uint8_t* const data,
    size_t* const data_index,
    size_t* const bits_in_current_chunk) {

        assert(data != NULL);
        assert(data_index != NULL);
        assert(bits_in_current_chunk != NULL);

        // Extract 8 bits from the current chunk
        uint8_t byte = chunk >> (*bits_in_current_chunk - 8); 
        data[*data_index] = byte;

        // Update current_chunk and bits_in_current_chunk for the remaining bits
        chunk &= (1 << (*bits_in_current_chunk - 8)) - 1; 
        *bits_in_current_chunk -= 8;

        // If there are remaining bits, store them in the next byte
        if (*bits_in_current_chunk > 0) {
            (*data_index)++;
            data[*data_index] = chunk; 
            *bits_in_current_chunk = 0; 
        }

        // If a full chunk was processed, move to the next byte
        if (*bits_in_current_chunk == 0) {
            (*data_index)++;
        }

}

void hx711_spi_put_next_chunk(
    uint8_t* const data,
    size_t* const data_index,
    uint16_t* const current_chunk,
    size_t* const bits_in_current_chunk) {

    assert(data != NULL);
    assert(data_index != NULL);
    assert(current_chunk != NULL);
    assert(bits_in_current_chunk != NULL);

    for (size_t i = 0; i < HX711_SPI_FRAME_DATA_SIZE && *bits_in_current_chunk > 0; ++i) {
        uint8_t bit = ((*current_chunk) >> (HX711_SPI_FRAME_DATA_SIZE - i - 1)) & 1; 
        if (*bits_in_current_chunk >= 8) {
            data[*data_index] |= (bit << (7 - (*bits_in_current_chunk - 8))); 
            (*bits_in_current_chunk) -= 8;
            (*data_index)++;
        } else {
            data[*data_index] |= (bit << (7 - *bits_in_current_chunk)); 
            *bits_in_current_chunk = 0; 
        }
    }

    (*current_chunk) <<= HX711_SPI_FRAME_DATA_SIZE;
    (*current_chunk) &= ((1 << HX711_SPI_FRAME_DATA_SIZE) - 1);

}
*/

void hx711_spi_send_data_chunked(
    spi_inst_t* const spi,
    const uint8_t* const data,
    const size_t lenBytes) {

        assert(spi != NULL);
        assert(data != NULL);
        assert(len > 0);
        assert(HX711_SPI_BITS_PER_TRANSFER == 16);

        hx711_spi_frame_t frame;
        const size_t numFrames = hx711_spi_calculate_frame_count(lenBytes * 8);
        uint16_t outbuffer;

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

void hx711_spi_receive_frame_blocking(
    spi_inst_t* const spi,
    hx711_spi_frame_t* const frame) {

        assert(spi != NULL);
        assert(frame != NULL);
        assert(HX711_SPI_BITS_PER_TRANSFER == 16);

        uint16_t inbuffer = 0;
        uint16_t outbuffer = 0;
        
        hx711_spi_frame_to_buffer(
            &HX711_SPI_NULL_FRAME,
            &outbuffer);

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
        while(!(f.not_null));

        *frame = f;

}

bool hx711_spi_try_receive_data(
    spi_inst_t* const spi,
    uint8_t* const data,
    const size_t expectedBitLen) {

        if(!spi_is_readable(spi)) {
            return false;
        }

        const size_t expectedFrames = hx711_spi_calculate_frame_count(expectedBitLen);
        size_t receivedFrames = 0;
        hx711_spi_frame_t frame = { 0 };

        hx711_spi_receive_frame_blocking(spi, &frame);

        if(!frame.is_first) {
            return false;
        }

        // set the first frame's data
        data[receivedFrames] = frame.data;
        receivedFrames++;

        while(!frame.is_last && receivedFrames < expectedFrames) {
            hx711_spi_receive_frame_blocking(spi, &frame);
            data[receivedFrames] = frame.data;
            receivedFrames++;
        }

        return true;

}

static size_t hx711_spi_receive_data_chunked(
    spi_inst_t* const spi,
    uint8_t* const data,
    const size_t expectedBitLen) {

        assert(spi != NULL);
        assert(data != NULL);
        assert(expectedBitLen > 0);

        hx711_spi_frame_t inframe;
        const size_t expectedFrameCount = hx711_spi_calculate_frame_count(expectedBitLen);
        size_t frameCount = 0;

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

            // is this correct?
            if(frameCount >= expectedFrameCount) {
                break;
            }

        }

        return frameCount;

}

void hx711_spi_master_init(
    hx711_spi_master_t* const hx_spi,
    const hx711_spi_master_config_t * const hx_spi_config) {

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

        const hx711_spi_request_t req = {
            .cmd = hx711_spi_command_change_gain,
            .gain = gain,
            .rate = rate
        };

        uint8_t buffer[HX711_SPI_REQUEST_TOTAL_SIZE_BYTES];

        hx711_spi_request_to_buffer(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            hx711_spi_send_data_chunked(
                hx_spi->_spi,
                buffer,
                HX711_SPI_REQUEST_TOTAL_SIZE_BYTES);
        );

}

int hx711_spi_master_get_control(
    hx711_spi_master_t* const hx_spi,
    hx711_spi_control_t* const ctrl) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);
        assert(ctrl != NULL);

        uint8_t buffer[HX711_SPI_CONTROL_TOTAL_BYTES];

        size_t bytesRead;

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            bytesRead = hx711_spi_receive_data_chunked(
                hx_spi->_spi,
                buffer,
                HX711_SPI_CONTROL_TOTAL_BYTES);
        );

        if(bytesRead != HX711_SPI_CONTROL_TOTAL_BYTES) {
            return PICO_ERROR_IO;
        }

        hx711_spi_buffer_to_control(
            buffer,
            ctrl);

        return PICO_OK;

}

int32_t hx711_spi_master_get_value_blocking(
    hx711_spi_master_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        hx711_spi_control_t ctrl = { 0 };

        while(hx711_spi_master_get_control(hx_spi, &ctrl) != PICO_OK) {
            if(!hx711_spi_control_ok(&ctrl)) {
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

        const hx711_spi_request_t req = {
            .cmd = hx711_spi_command_change_power_state,
            .power_state = true,
            .gain = gain,
            .rate = rate
        };

        uint8_t buffer[HX711_SPI_REQUEST_TOTAL_SIZE_BYTES];

        hx711_spi_request_to_buffer(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            hx711_spi_send_data_chunked(
                hx_spi->_spi,
                buffer,
                HX711_SPI_REQUEST_TOTAL_SIZE_BYTES);
        );

}

void hx711_spi_master_power_down(
    hx711_spi_master_t* const hx_spi) {

        assert(hx_spi != NULL);
        assert(hx_spi->_spi != NULL);

        const hx711_spi_request_t req = {
            .cmd = hx711_spi_command_change_power_state,
            .power_state = false
        };

        uint8_t buffer[HX711_SPI_REQUEST_TOTAL_SIZE_BYTES];

        hx711_spi_request_to_buffer(
            &req,
            buffer);

        HX711_SPI_ATOMIC(hx_spi->_csn_pin, 
            hx711_spi_send_data_chunked(
                hx_spi->_spi,
                buffer,
                HX711_SPI_REQUEST_TOTAL_SIZE_BYTES);
        );

}
