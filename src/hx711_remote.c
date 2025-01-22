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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/hx711.h"
#include "../include/hx711_remote.h"
#include "../include/util.h"

const hx711_remote_control_t HX711_REMOTE_CONTROL_DEFAULTS = {
    .ready_state = false,
    .new_value_state = false,
    .power_state = false,
    .gain = hx711_gain_128,
    .rate = hx711_rate_10,
    .value = 0
};

void hx711_remote_control_get_defaults(
    hx711_remote_control_t* const ctrl) {
        assert(ctrl != NULL);
        *ctrl = HX711_REMOTE_CONTROL_DEFAULTS;
}

void hx711_remote_serialise_control(
    const hx711_remote_control_t* const ctrl,
    uint8_t* const buffer) {

        assert(ctrl != NULL);
        assert(buffer != NULL);

        memset(buffer, 0, HX711_REMOTE_CONTROL_TOTAL_BYTES);

        buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_READY_STATE_OFFSET,
            HX711_REMOTE_CONTROL_READY_STATE_SIZE,
            (uint8_t)ctrl->ready_state);

        buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_NEW_VALUE_STATE_OFFSET,
            HX711_REMOTE_CONTROL_NEW_VALUE_STATE_SIZE,
            (uint8_t)ctrl->new_value_state);

        buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_POWER_STATE_OFFSET,
            HX711_REMOTE_CONTROL_POWER_STATE_SIZE,
            (uint8_t)ctrl->power_state);

        buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_GAIN_OFFSET,
            HX711_REMOTE_CONTROL_GAIN_SIZE,
            (uint8_t)ctrl->gain);

        buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_RATE_OFFSET,
            HX711_REMOTE_CONTROL_RATE_SIZE,
            (uint8_t)ctrl->rate);

        hx711_remote_value_to_array(
            ctrl->value,
            &buffer[HX711_REMOTE_CONTROL_DATA_OFFSET_BYTES]);

}

void hx711_remote_deserialise_control(
    const uint8_t* const buffer,
    hx711_remote_control_t* const ctrl) {

        assert(buffer != NULL);
        assert(ctrl != NULL);

        ctrl->ready_state = (bool)util_get_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_READY_STATE_OFFSET,
            HX711_REMOTE_CONTROL_READY_STATE_SIZE);

        ctrl->new_value_state = (bool)util_get_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_NEW_VALUE_STATE_OFFSET,
            HX711_REMOTE_CONTROL_NEW_VALUE_STATE_SIZE);

        ctrl->power_state = (bool)util_get_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_POWER_STATE_OFFSET,
            HX711_REMOTE_CONTROL_POWER_STATE_SIZE);

        ctrl->gain = (hx711_gain_t)util_get_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_GAIN_OFFSET,
            HX711_REMOTE_CONTROL_GAIN_SIZE);

        assert(hx711_is_gain_valid(ctrl->gain));

        ctrl->rate = (hx711_rate_t)util_get_bits8(
            buffer[HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES],
            HX711_REMOTE_CONTROL_RATE_OFFSET,
            HX711_REMOTE_CONTROL_RATE_SIZE);

        assert(hx711_is_rate_valid(ctrl->rate));

        ctrl->value = hx711_remote_array_to_value(
            &buffer[HX711_REMOTE_CONTROL_DATA_OFFSET_BYTES]);

        assert(hx711_is_value_valid(ctrl->value));

}

void hx711_remote_serialise_request(
    const hx711_remote_request_t* const req,
    uint8_t* const buffer) {

        assert(req != NULL);
        assert(buffer != NULL);

        memset(buffer, 0, HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES);

        buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES],
            HX711_REMOTE_REQUEST_COMMAND_OFFSET,
            HX711_REMOTE_REQUEST_COMMAND_SIZE,
            (uint8_t)req->cmd);

        buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES],
            HX711_REMOTE_REQUEST_POWER_STATE_OFFSET,
            HX711_REMOTE_REQUEST_POWER_STATE_SIZE,
            (uint8_t)req->power_state);

        buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES],
            HX711_REMOTE_REQUEST_GAIN_OFFSET,
            HX711_REMOTE_REQUEST_GAIN_SIZE,
            (uint8_t)req->gain);

        buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES] = util_set_bits8(
            buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES],
            HX711_REMOTE_REQUEST_RATE_OFFSET,
            HX711_REMOTE_REQUEST_RATE_SIZE,
            (uint8_t)req->rate);

}

void hx711_remote_deserialise_request(
    const uint8_t* const buffer,
    hx711_remote_request_t* const req) {

        assert(buffer != NULL);
        assert(req != NULL);

        req->cmd = (hx711_remote_command_t)util_get_bits8(
            buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES],
            HX711_REMOTE_REQUEST_COMMAND_OFFSET,
            HX711_REMOTE_REQUEST_COMMAND_SIZE);

        assert(hx711_remote_command_is_valid(req->cmd));

        req->power_state = (bool)util_get_bits8(
            buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES],
            HX711_REMOTE_REQUEST_POWER_STATE_OFFSET,
            HX711_REMOTE_REQUEST_POWER_STATE_SIZE);

        req->gain = (hx711_gain_t)util_get_bits8(
            buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES],
            HX711_REMOTE_REQUEST_GAIN_OFFSET,
            HX711_REMOTE_REQUEST_GAIN_SIZE);

        assert(hx711_is_gain_valid(req->gain));

        req->rate = (hx711_rate_t)util_get_bits8(
            buffer[HX711_REMOTE_REQUEST_OFFSET_BYTES],
            HX711_REMOTE_REQUEST_RATE_OFFSET,
            HX711_REMOTE_REQUEST_RATE_SIZE);

        assert(hx711_is_rate_valid(req->rate));

}
