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

#include <stdint.h>
#include "hardware/spi.h"
#include "hx711.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HX711_SPI_BAUD_RATE 1000000

typedef enum {
    hx711_spi_command_none = 0,
    hx711_spi_command_power_up = 1,
    hx711_spi_command_power_down = 2,
    hx711_spi_command_set_gain = 3,
    hx711_spi_command_get_value = 4
} hx711_spi_command_t;

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

void hx711_spi_master_init(
    hx711_spi_master_t* const hx_spi,
    const hx711_spi_master_config_t * const hx_spi_config);

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
    const hx711_gain_t gain);

/**
 * @brief Obtains a value from the HX711. Blocks until a value
 * is available.
 * 
 * @param hx_spi 
 * @return int32_t 
 */
int32_t hx711_spi_master_get_value(
    hx711_spi_master_t* const hx_spi);

/**
 * @brief Power up the HX711 with an initial gain.
 * 
 * @param hx_spi 
 * @param gain 
 */
void hx711_spi_master_power_up(
    hx711_spi_master_t* const hx_spi,
    const hx711_gain_t gain);

/**
 * @brief Power down the HX711.
 * 
 * @param hx_spi 
 */
void hx711_spi_master_power_down(
    hx711_spi_master_t* const hx_spi);

/**
 * @brief Checks whether the given gain value is valid for
 * transmissions across SPI.
 * 
 * @param spi_gain 
 * @return true 
 * @return false 
 */
bool hx711_spi_is_spi_gain_valid(
    const uint8_t spi_gain);

/**
 * @brief Converts a hx711_gain_t to a value suitable for
 * transmission across SPI.
 * 
 * @param gain 
 * @return uint8_t 
 */
uint8_t hx711_spi_gain_to_spi_gain(
    const hx711_gain_t gain);

/**
 * @brief Converts a gain value obtained via SPI to a
 * hx711_gain_t.
 * 
 * @param spi_gain 
 * @return hx711_gain_t 
 */
hx711_gain_t hx711_spi_spi_gain_to_gain(
    const uint8_t spi_gain);

#ifdef __cplusplus
}
#endif

#endif
