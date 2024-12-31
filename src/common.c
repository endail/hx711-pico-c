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
#include <stddef.h>
#include <hardware/i2c.h>
#include <hardware/pio.h>
#include "../include/common.h"
#include "../include/hx711.h"
#include "../include/hx711_reader.pio.h"
#include "../include/hx711_multi.h"
#include "../include/hx711_multi_awaiter.pio.h"
#include "../include/hx711_multi_reader.pio.h"
#include "../include/hx711_i2c_master.h"
#include "../include/hx711_i2c_slave.h"
#include "../include/hx711_spi_master.h"
#include "../include/hx711_spi_slave.h"

const hx711_config_t HX711__DEFAULT_CONFIG = {
    .clock_pin = 0,
    .data_pin = 0,
    .pio = pio0,
    .pio_init = hx711_reader_pio_init,
    .reader_prog = &hx711_reader_program,
    .reader_prog_init = hx711_reader_program_init
};

const hx711_multi_config_t HX711__MULTI_DEFAULT_CONFIG = {
    .clock_pin = 0,
    .data_pin_base = 0,
    .chips_len = 0,
    .pio_irq_index = HX711_MULTI_ASYNC_PIO_IRQ_IDX,
    .dma_irq_index = HX711_MULTI_ASYNC_DMA_IRQ_IDX,
    .pio = pio0,
    .pio_init = hx711_multi_pio_init,
    .awaiter_prog = &hx711_multi_awaiter_program,
    .awaiter_prog_init = hx711_multi_awaiter_program_init,
    .reader_prog = &hx711_multi_reader_program,
    .reader_prog_init = hx711_multi_reader_program_init
};

const hx711_i2c_master_config_t HX711__I2C_MASTER_DEFAULT_CONFIG = {
    .scl_pin = HX711_I2C_DEFAULT_SCL_PIN,
    .sda_pin = HX711_I2C_DEFAULT_SDA_PIN,
    .i2c = HX711_I2C_DEFAULT_INST,
    .baud_rate = HX711_I2C_DEFAULT_BAUD_RATE,
    .addr = HX711_I2C_DEFAULT_I2C_ADDR
};

const hx711_i2c_slave_config_t HX711__I2C_SLAVE_DEFAULT_CONFIG = {
    .scl_pin = HX711_I2C_DEFAULT_SCL_PIN,
    .sda_pin = HX711_I2C_DEFAULT_SDA_PIN,
    .i2c = HX711_I2C_DEFAULT_INST,
    .baud_rate = HX711_I2C_DEFAULT_BAUD_RATE,
    .addr = HX711_I2C_DEFAULT_I2C_ADDR,
    .hx = NULL
};

const hx711_spi_master_config_t HX711__SPI_MASTER_DEFAULT_CONFIG = {
    .sck_pin = HX711_SPI_DEFAULT_SCK_PIN,
    .csn_pin = HX711_SPI_DEFAULT_CSN_PIN,
    .rx_pin = HX711_SPI_DEFAULT_RX_PIN,
    .tx_pin = HX711_SPI_DEFAULT_TX_PIN,
    .spi = HX711_SPI_DEFAULT_INST,
    .baud_rate = HX711_SPI_BAUD_RATE
};

const hx711_spi_slave_config_t HX711__SPI_SLAVE_DEFAULT_CONFIG = {
    .sck_pin = HX711_SPI_DEFAULT_SCK_PIN,
    .csn_pin = HX711_SPI_DEFAULT_CSN_PIN,
    .rx_pin = HX711_SPI_DEFAULT_RX_PIN,
    .tx_pin = HX711_SPI_DEFAULT_TX_PIN,
    .spi = HX711_SPI_DEFAULT_INST,
    .baud_rate = HX711_SPI_BAUD_RATE,
    .hx = NULL
};

void hx711_get_default_config(
    hx711_config_t* const cfg) {
        assert(cfg != NULL);
        *cfg = HX711__DEFAULT_CONFIG;
}

void hx711_multi_get_default_config(
    hx711_multi_config_t* const cfg) {
        assert(cfg != NULL);
        *cfg = HX711__MULTI_DEFAULT_CONFIG;
}

void hx711_i2c_master_get_default_config(
    hx711_i2c_master_config_t* const cfg) {
        assert(cfg != NULL);
        *cfg = HX711__I2C_MASTER_DEFAULT_CONFIG;
}

void hx711_i2c_slave_get_default_config(
    hx711_i2c_slave_config_t* const cfg) {
        assert(cfg != NULL);
        *cfg = HX711__I2C_SLAVE_DEFAULT_CONFIG;
}

void hx711_spi_master_get_default_config(
    hx711_spi_master_config_t* const cfg) {
        assert(cfg != NULL);
        *cfg = HX711__SPI_MASTER_DEFAULT_CONFIG;
}

void hx711_spi_slave_get_default_config(
    hx711_spi_slave_config_t* const cfg) {
        assert(cfg != NULL);
        *cfg = HX711__SPI_SLAVE_DEFAULT_CONFIG;
}
