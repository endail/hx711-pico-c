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

#include <stdlib.h>
#include <stdio.h>
#include "pico/stdio.h"
#include "tusb.h"
#include "../include/common.h"

int main(void) {

    stdio_init_all();

    while (!tud_cdc_connected()) {
        sleep_ms(1);
    }

    hx711_spi_master_config_t spicfg;
    hx711_spi_master_get_default_config(&spicfg);

    hx711_spi_master_t hxspi;

    hx711_spi_master_init(&hxspi, &spicfg);
    
    //hx711_spi_master_power_up(&hxspi, hx711_gain_128);
    //hx711_wait_settle(hx711_rate_80);
    
    //int32_t val;

    while(true) {
        //int32_t val = hx711_spi_master_get_value(&hxspi);
        //printf("Received value: %li -> %x %x %x %x\n",
        //    val,
        //    (uint8_t)(val >> 24),
        //    (uint8_t)((val >> 16) & 0xff),
        //    (uint8_t)((val >> 8) & 0xff),
        //    (uint8_t)(val & 0xff));

        //if(hx711_spi_master_get_value(&hxspi, &val)) {
        //    printf("%li\n", val);
        //}
        //else {
        //    printf("Checksum fail!\n");
        //}

        //printf("%li\n", hx711_spi_master_get_value(&hxspi));
        //hx711_spi_master_get_value(&hxspi);
        //sleep_ms(1000);
    }
    hx711_spi_master_close(&hxspi);

    while(1);

    return EXIT_SUCCESS;

}
