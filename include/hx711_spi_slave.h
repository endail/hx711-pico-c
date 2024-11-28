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

#ifndef HX711_SPI_SLAVE_H_A013478A_24EE_4581_AF94_BC52198CFF1D
#define HX711_SPI_SLAVE_H_A013478A_24EE_4581_AF94_BC52198CFF1D

#include <stdint.h>
#include "hardware/spi.h"
#include "hx711.h"
#include "hx711_spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

    uint _rx_pin;
    uint _sck_pin;
    uint _tx_pin;
    uint _csn_pin;

    spi_inst_t* _spi;
    uint _baud_rate;

    hx711_t* _hx;

} hx711_spi_slave_t;

typedef struct {

    uint rx_pin;
    uint sck_pin;
    uint tx_pin;
    uint csn_pin;

    spi_inst_t* spi;
    uint baud_rate;

    hx711_t* hx;

} hx711_spi_slave_config_t;

/**
 * @brief Initialise SPI slave device.
 * 
 * @param hx_spi 
 * @param hx_spi_config 
 */
void hx711_spi_slave_init(
    hx711_spi_slave_t* const hx_spi,
    const hx711_spi_slave_config_t * const hx_spi_config);

/**
 * @brief Stop SPI communication.
 * 
 * @param hx_spi 
 */
void hx711_spi_slave_close(hx711_spi_slave_t* const hx_spi);

/**
 * @brief Listen for incoming SPI messages and respond.
 * 
 * @param hx_spi 
 */
void hx711_spi_slave_listen(hx711_spi_slave_t* const hx_spi);

#ifdef __cplusplus
}
#endif

#endif
