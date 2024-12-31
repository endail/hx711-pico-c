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

#ifndef HX711_SPI_MASTER_H_20E2043E_D984_444F_A079_562B3C56AC12
#define HX711_SPI_MASTER_H_20E2043E_D984_444F_A079_562B3C56AC12

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/divider.h>
#include <pico/platform.h>
#include <pico/types.h>
#include "hx711.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HX711_SPI_DEFAULT_SCK_PIN                   PICO_DEFAULT_SPI_SCK_PIN
#define HX711_SPI_DEFAULT_CSN_PIN                   PICO_DEFAULT_SPI_CSN_PIN
#define HX711_SPI_DEFAULT_RX_PIN                    PICO_DEFAULT_SPI_RX_PIN
#define HX711_SPI_DEFAULT_TX_PIN                    PICO_DEFAULT_SPI_TX_PIN
#define HX711_SPI_DEFAULT_INST                      spi_default
#define HX711_SPI_BAUD_RATE                         4000000u
#define HX711_SPI_BITS_PER_TRANSFER                 16u

#define HX711_SPI_FRAME_FLAGS_OFFSET                0u
#define HX711_SPI_FRAME_FLAGS_NOT_NULL_OFFSET       0u
#define HX711_SPI_FRAME_FLAGS_FIRST_OFFSET          1u
#define HX711_SPI_FRAME_FLAGS_LAST_OFFSET           2u
#define HX711_SPI_FRAME_FLAGS_CONTINUING_OFFSET     3u
#define HX711_SPI_FRAME_DATA_OFFSET                 7u

#define HX711_SPI_FRAME_SIZE                        HX711_SPI_BITS_PER_TRANSFER
#define HX711_SPI_FRAME_FLAGS_SIZE                  8u
#define HX711_SPI_FRAME_FLAGS_NOT_NULL_SIZE         1u
#define HX711_SPI_FRAME_FLAGS_FIRST_SIZE            1u
#define HX711_SPI_FRAME_FLAGS_LAST_SIZE             1u
#define HX711_SPI_FRAME_FLAGS_CONTINUING_SIZE       1u
#define HX711_SPI_FRAME_DATA_SIZE                   8u

#define HX711_SPI_CONTROL_METADATA_OFFSET_BYTES     0u
#define HX711_SPI_CONTROL_READY_STATE_OFFSET        0u
#define HX711_SPI_CONTROL_NEW_VALUE_STATE_OFFSET    1u
#define HX711_SPI_CONTROL_POWER_STATE_OFFSET        2u
#define HX711_SPI_CONTROL_GAIN_OFFSET               3u
#define HX711_SPI_CONTROL_RATE_OFFSET               5u
#define HX711_SPI_CONTROL_DATA_OFFSET               8u
#define HX711_SPI_CONTROL_DATA_OFFSET_BYTES         1u

#define HX711_SPI_CONTROL_READY_STATE_SIZE          1u
#define HX711_SPI_CONTROL_NEW_VALUE_STATE_SIZE      1u
#define HX711_SPI_CONTROL_POWER_STATE_SIZE          1u
#define HX711_SPI_CONTROL_GAIN_SIZE                 2u
#define HX711_SPI_CONTROL_RATE_SIZE                 1u

#define HX711_SPI_CONTROL_DATA_SIZE_BITS            HX711_READ_BITS
#define HX711_SPI_CONTROL_DATA_SIZE_BYTES           3u
#define HX711_SPI_CONTROL_METADATA_SIZE_BITS        6u
#define HX711_SPI_CONTROL_METADATA_SIZE_BYTES       1u
#define HX711_SPI_CONTROL_TOTAL_BYTES               4u

#define HX711_SPI_REQUEST_OFFSET_BYTES              0u
#define HX711_SPI_REQUEST_COMMAND_OFFSET            0u
#define HX711_SPI_REQUEST_POWER_STATE_OFFSET        2u
#define HX711_SPI_REQUEST_GAIN_OFFSET               3u
#define HX711_SPI_REQUEST_RATE_OFFSET               5u

#define HX711_SPI_REQUEST_COMMAND_SIZE              2u
#define HX711_SPI_REQUEST_POWER_STATE_SIZE          1u
#define HX711_SPI_REQUEST_GAIN_SIZE                 2u
#define HX711_SPI_REQUEST_RATE_SIZE                 1u
#define HX711_SPI_REQUEST_TOTAL_SIZE_BYTES          1u

#define HX711_SPI_ATOMIC(CSN_PIN, ...) \
    do { \
        gpio_put(CSN_PIN, false); \
        __VA_ARGS__ \
        gpio_put(CSN_PIN, true); \
    } while (0)

/**
 * SPI Frame Structure (16 bits)
 * | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * 0th = not null frame
 * 1th = first frame
 * 2nd = last frame
 * 3rd = continuing frame
 * 4-12 = data
 */
/*typedef enum {
    hx711_spi_frame_flags_none =                1
    hx711_spi_frame_flags_first =               2,
    hx711_spi_frame_flags_continuing =          3,
    hx711_spi_frame_flags_last =                4,
} hx711_spi_frame_flags_t;*/

typedef enum {
    hx711_spi_command_none =                    0,
    hx711_spi_command_change_power_state =      1,
    hx711_spi_command_change_gain =             2,
    hx711_spi_command_get_value =               3
} hx711_spi_command_t;

typedef struct {
    bool not_null;
    bool is_first;
    bool is_last;
    bool is_continuing;
    bool unused_4;
    bool unused_5;
    bool unused_6;
    bool unused_7;
    uint8_t data;
} hx711_spi_frame_t;

typedef struct {
    bool ready_state;
    bool new_value_state;
    bool power_state;
    hx711_gain_t gain;
    hx711_rate_t rate;
    int32_t value;
} hx711_spi_control_t;

typedef struct {
    hx711_spi_command_t cmd;
    bool power_state;
    hx711_gain_t gain;
    hx711_rate_t rate;
} hx711_spi_request_t;

typedef struct {

    uint _rx_pin;
    uint _sck_pin;
    uint _tx_pin;
    uint _csn_pin;

    spi_inst_t* _spi;
    uint _baud_rate;

} hx711_spi_master_t;

typedef struct {

    uint rx_pin;
    uint sck_pin;
    uint tx_pin;
    uint csn_pin;

    spi_inst_t* spi;
    uint baud_rate;

} hx711_spi_master_config_t;

extern const hx711_spi_frame_t HX711_SPI_NULL_FRAME;

/**
 * @brief Convert a 32-bit value from a HX711 to
 * a 3-byte array.
 * 
 * @param val 
 * @param arr 
 */
inline void hx711_spi_value_to_array(
    const int32_t val,
    uint8_t* const arr) {
        assert(arr != NULL);
        assert(hx711_is_value_valid(val));
        const int32_t extval = (val << 8) >> 8;
        arr[0] = (uint8_t)extval;
        arr[1] = (uint8_t)(extval >> 8);
        arr[2] = (uint8_t)(extval >> 16);
}

/**
 * @brief Convert a 3-byte array containing a HX711
 * value to a 32-bit integer.
 * 
 * @param arr 
 * @return int32_t 
 */
inline int32_t hx711_spi_array_to_value(
    const uint8_t* const arr) {
        assert(arr != NULL);
        int32_t val = arr[0] | (arr[1] << 8) | (arr[2] << 16);
        val = (val << 8) >> 8;
        assert(hx711_is_value_valid(val));
        return val;
}

static void hx711_spi_frame_to_buffer(
    const hx711_spi_frame_t* const frame,
    uint16_t* const buffer);

static void hx711_spi_buffer_to_frame(
    const uint16_t* const buffer,
    hx711_spi_frame_t* const frame);

void hx711_spi_control_to_buffer(
    const hx711_spi_control_t* const ctrl,
    uint8_t* const buffer);

void hx711_spi_buffer_to_control(
    const uint8_t* const buffer,
    hx711_spi_control_t* const ctrl);

void hx711_spi_request_to_buffer(
    const hx711_spi_request_t* const ctrl,
    uint8_t* const buffer);

void hx711_spi_buffer_to_request(
    const uint8_t* const buffer,
    hx711_spi_request_t* const req);

inline bool hx711_spi_control_ok(
    const hx711_spi_control_t* const ctrl) {
        return ctrl->ready_state &&
            ctrl->power_state &&
            ctrl->new_value_state;
}

inline static size_t hx711_spi_calculate_frame_count(
    const size_t dataBitsLen) {

        uint32_t rem;
        uint32_t result;

        result = divmod_u32u32_rem(
            dataBitsLen * 8,
            HX711_SPI_FRAME_DATA_SIZE,
            &rem);

        if(rem > 0) {
            result++;
        }

        return result;

        //const size_t totalBits = data_len * 8;
        //size_t chunkCount = totalBits / HX711_SPI_FRAME_DATA_SIZE;
        //if(totalBits % HX711_SPI_FRAME_DATA_SIZE > 0) {
        //    chunkCount++;
        //}
        //return chunkCount;

}

void hx711_spi_send_data_chunked(
    spi_inst_t* const spi,
    const uint8_t* const data,
    const size_t lenBytes);

static void hx711_spi_receive_frame_blocking(
    spi_inst_t* const spi,
    hx711_spi_frame_t* const frame);

static void hx711_spi_receive_first_frame_blocking(
    spi_inst_t* const spi,
    hx711_spi_frame_t* const frame);

bool hx711_spi_try_receive_data(
    spi_inst_t* const spi,
    uint8_t* const data,
    const size_t expectedBitLen);

static size_t hx711_spi_receive_data_chunked(
    spi_inst_t* const spi,
    uint8_t* const data,
    const size_t expectedBitLen);

void hx711_spi_master_init(
    hx711_spi_master_t* const hx_spi,
    const hx711_spi_master_config_t* const hx_spi_config);

/**
 * @brief Stop SPI communication.
 * 
 * @param hx_spi 
 */
void hx711_spi_master_close(
    hx711_spi_master_t* const hx_spi);

/**
 * @brief Sets the HX711 gain.
 * 
 * @param hx_spi 
 * @param gain 
 */
void hx711_spi_master_set_gain(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain,
    const hx711_rate_t rate);

int hx711_spi_master_get_control(
    hx711_spi_master_t* const hx_spi,
    hx711_spi_control_t* const ctrl);

int32_t hx711_spi_master_get_value_blocking(
    hx711_spi_master_t* const hx_spi);

/**
 * @brief Power up the HX711 with an initial gain.
 * 
 * @param hx_spi 
 * @param gain 
 */
void hx711_spi_master_power_up(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain,
    const hx711_rate_t rate);

/**
 * @brief Power down the HX711.
 * 
 * @param hx_spi 
 */
void hx711_spi_master_power_down(
    hx711_spi_master_t* const hx_spi);

#ifdef __cplusplus
}
#endif

#endif