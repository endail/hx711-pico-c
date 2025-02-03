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
#include "hx711_remote.h"
#include "spifixedframe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HX711_SPI_DEFAULT_SCK_PIN                   PICO_DEFAULT_SPI_SCK_PIN
#define HX711_SPI_DEFAULT_CSN_PIN                   PICO_DEFAULT_SPI_CSN_PIN
#define HX711_SPI_DEFAULT_RX_PIN                    PICO_DEFAULT_SPI_RX_PIN
#define HX711_SPI_DEFAULT_TX_PIN                    PICO_DEFAULT_SPI_TX_PIN
#define HX711_SPI_DEFAULT_INST                      spi_default
#define HX711_SPI_BAUD_RATE                         4000000u
#define HX711_SPI_SPI_DATA_BITS                     SPIFIXEDFRAME_TOTAL_BITS
#define HX711_SPI_SPI_POLARITY                      SPI_CPOL_0
#define HX711_SPI_SPI_PHASE                         SPI_CPHA_0
#define HX711_SPI_SPI_ORDER                         SPI_MSB_FIRST

#define HX711_SPI_REMOTE_REQUEST_CRC_SIZE_BYTES     sizeof(uint8_t)
#define HX711_SPI_REMOTE_CONTROL_CRC_SIZE_BYTES     sizeof(uint32_t)

#define HX711_SPI_REMOTE_REQUEST_TOTAL_BYTES        ((HX711_REMOTE_REQUEST_TOTAL_SIZE_BYTES) + (HX711_SPI_REMOTE_REQUEST_CRC_SIZE_BYTES))
#define HX711_SPI_REMOTE_CONTROL_TOTAL_BYTES        ((HX711_REMOTE_CONTROL_TOTAL_BYTES) + (HX711_SPI_REMOTE_CONTROL_CRC_SIZE_BYTES))

#define HX711_SPI_CRC8_POLYNOMIAL                   0x07u
#define HX711_SPI_CRC32_POLYNOMIAL                  0xedb88320u

#define HX711_SPI_ATOMIC(CSN_PIN, ...) \
    do { \
        gpio_put(CSN_PIN, false); \
        __VA_ARGS__ \
        gpio_put(CSN_PIN, true); \
    } while(0)

#define HX711_SPI_MASTER_SPI_TIMEOUT_US             1000000u

typedef enum {
    HX711_SPI_ERROR_OK =                    0,
    HX711_SPI_ERROR_GENERIC =               1,
    HX711_SPI_ERROR_UNKNOWN =               2,
    HX711_SPI_ERROR_CRC_FAIL =              3,
    HX711_SPI_ERROR_SPI_SEND_FAIL =         4,
    HX711_SPI_ERROR_SPI_RECV_FAIL =         5
} hx711_spi_error_t;

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

void hx711_spi_serialise_request(
    const hx711_remote_request_t* const req,
    uint8_t* const buffer);

void hx711_spi_serialise_control(
    const hx711_remote_control_t* const ctrl,
    uint8_t* const buffer);

hx711_spi_error_t hx711_spi_deserialise_request(
    const uint8_t* const buffer,
    hx711_remote_request_t* const req);

hx711_spi_error_t hx711_spi_deserialise_control(
    const uint8_t* const buffer,
    hx711_remote_control_t* const ctrl);

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
hx711_spi_error_t hx711_spi_master_set_gain(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain,
    const hx711_rate_t rate);

hx711_spi_error_t hx711_spi_master_get_control(
    hx711_spi_master_t* const hx_spi,
    hx711_remote_control_t* const ctrl);

int32_t hx711_spi_master_get_value_blocking(
    hx711_spi_master_t* const hx_spi);

/**
 * @brief Power up the HX711 with an initial gain.
 * 
 * @param hx_spi 
 * @param gain 
 */
hx711_spi_error_t hx711_spi_master_power_up(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain,
    const hx711_rate_t rate);

/**
 * @brief Power down the HX711.
 * 
 * @param hx_spi 
 */
hx711_spi_error_t hx711_spi_master_power_down(
    hx711_spi_master_t* const hx_spi);

#ifdef __cplusplus
}
#endif

#endif