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

#ifndef HX711_REMOTE_H_880BAB7A_CA45_4D49_AA3C_C7F5121F9D58
#define HX711_REMOTE_H_880BAB7A_CA45_4D49_AA3C_C7F5121F9D58

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "hx711.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Control bits structure
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * 0th bit = ready state
 * 1th bit = new value state
 * 2nd bit = power state
 * 3rd bit = gain
 * 4th bit = gain
 * 5th bit = rate
 * 6th bit = unused
 * 7th bit = unused
 * ---------------- (byte boundary)
 * 8th .. 31th bit = value bits
 */

#define HX711_REMOTE_CONTROL_METADATA_OFFSET_BYTES      0u
#define HX711_REMOTE_CONTROL_READY_STATE_OFFSET         0u
#define HX711_REMOTE_CONTROL_NEW_VALUE_STATE_OFFSET     1u
#define HX711_REMOTE_CONTROL_POWER_STATE_OFFSET         2u
#define HX711_REMOTE_CONTROL_GAIN_OFFSET                3u
#define HX711_REMOTE_CONTROL_RATE_OFFSET                5u
#define HX711_REMOTE_CONTROL_DATA_OFFSET                8u
#define HX711_REMOTE_CONTROL_DATA_OFFSET_BYTES          1u

#define HX711_REMOTE_CONTROL_READY_STATE_SIZE           1u
#define HX711_REMOTE_CONTROL_NEW_VALUE_STATE_SIZE       1u
#define HX711_REMOTE_CONTROL_POWER_STATE_SIZE           1u
#define HX711_REMOTE_CONTROL_GAIN_SIZE                  2u
#define HX711_REMOTE_CONTROL_RATE_SIZE                  1u

#define HX711_REMOTE_CONTROL_METADATA_SIZE_BITS         6u
#define HX711_REMOTE_CONTROL_METADATA_SIZE_BYTES        1u
#define HX711_REMOTE_CONTROL_DATA_SIZE_BITS             HX711_READ_BITS
#define HX711_REMOTE_CONTROL_DATA_SIZE_BYTES            3u
#define HX711_REMOTE_CONTROL_TOTAL_BYTES                4u

/**
 * @brief Request bits structure
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * 0th bit = command bit
 * 1th bit = command bit
 * 2nd bit = power state bit
 * 3rd bit = gain bit
 * 4th bit = gain bit
 * 5th bit = rate bit
 * 6th bit = unused
 * 7th bit = unused
 */

#define HX711_REMOTE_REQUEST_OFFSET_BYTES               0u
#define HX711_REMOTE_REQUEST_COMMAND_OFFSET             0u
#define HX711_REMOTE_REQUEST_POWER_STATE_OFFSET         2u
#define HX711_REMOTE_REQUEST_GAIN_OFFSET                3u
#define HX711_REMOTE_REQUEST_RATE_OFFSET                5u

#define HX711_REMOTE_REQUEST_COMMAND_SIZE               2u
#define HX711_REMOTE_REQUEST_POWER_STATE_SIZE           1u
#define HX711_REMOTE_REQUEST_GAIN_SIZE                  2u
#define HX711_REMOTE_REQUEST_RATE_SIZE                  1u
#define HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES           1u

typedef enum {
    hx711_remote_command_none =                         0u,
    hx711_remote_command_change_power_state =           1u,
    hx711_remote_command_change_gain =                  2u,
    hx711_remote_command_get_control =                    3u
} hx711_remote_command_t;

typedef struct {
    bool ready_state;
    bool new_value_state;
    bool power_state;
    hx711_gain_t gain;
    hx711_rate_t rate;
    int32_t value;
    uint32_t crc;
} hx711_remote_control_t;

typedef struct {
    hx711_remote_command_t cmd;
    bool power_state;
    hx711_gain_t gain;
    hx711_rate_t rate;
    uint8_t crc;
} hx711_remote_request_t;

extern const hx711_remote_control_t HX711_REMOTE_CONTROL_DEFAULTS;

/**
 * @brief Convert a 32-bit value from a HX711 to
 * a 3-byte array.
 * 
 * @param val 
 * @param arr 
 */
void hx711_remote_value_to_array(
    const int32_t val,
    uint8_t* const arr);

/**
 * @brief Convert a 3-byte array containing a HX711
 * value to a 32-bit integer.
 * 
 * @param arr 
 * @return int32_t 
 */
int32_t hx711_remote_array_to_value(
    const uint8_t* const arr);

void hx711_remote_control_get_defaults(
    hx711_remote_control_t* const ctrl);

void hx711_remote_serialise_control(
    const hx711_remote_control_t* const ctrl,
    uint8_t* const buffer);

void hx711_remote_deserialise_control(
    const uint8_t* const buffer,
    hx711_remote_control_t* const ctrl);

void hx711_remote_serialise_request(
    const hx711_remote_request_t* const req,
    uint8_t* const buffer);

void hx711_remote_deserialise_request(
    const uint8_t* const buffer,
    hx711_remote_request_t* const req);

inline bool hx711_remote_control_ok(
    const hx711_remote_control_t* const ctrl) {
        assert(ctrl != NULL);
        return ctrl->ready_state &&
            ctrl->power_state &&
            ctrl->new_value_state;
}

inline bool hx711_remote_command_is_valid(
    const hx711_remote_command_t cmd) {
        return (uint8_t)cmd <= hx711_remote_command_get_control;
}

#ifdef __cplusplus
}
#endif

#endif