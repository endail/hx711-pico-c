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
#include <pico/stdio.h>
#include <tusb.h>
#include "../include/common.h"

#define PROG_HX_CLOCK_PIN 14
#define PROG_HX_DATA_PIN 15
#define PROG_HX_GAIN hx711_gain_128
#define PROG_HX_RATE hx711_rate_80

int main(void) {

    stdio_init_all();

    while (!tud_cdc_connected()) {
        sleep_ms(1);
    }

    hx711_t hx;
    hx711_config_t hxcfg;
    hx711_i2c_slave_t hxi2c;
    hx711_i2c_slave_config_t i2ccfg;

    // set hx711 config
    hx711_get_default_config(&hxcfg);
    hxcfg.clock_pin = PROG_HX_CLOCK_PIN;
    hxcfg.data_pin = PROG_HX_DATA_PIN;

    // set slave config
    hx711_i2c_slave_get_default_config(&i2ccfg);
    i2ccfg.hx = &hx;

    // init hx711
    hx711_init(&hx, &hxcfg);

    // init slave
    hx711_i2c_slave_init(&hxi2c, &i2ccfg);

    // power up and settle
    hx711_power_up(&hx, PROG_HX_GAIN);
    hx711_wait_settle(PROG_HX_RATE);

    // set new control bits
    const hx711_remote_control_t ctrlconf = {
        .ready_state = true,
        .new_value_state = false,
        .power_state = true,
        .gain = PROG_HX_GAIN,
        .rate = PROG_HX_RATE
    };

    hx711_i2c_slave_set_control(&hxi2c, &ctrlconf);

    // slave interrupt will be effective at this point
    printf("Ready\n");

    // get new values and save them
    hx711_i2c_slave_update_loop(&hxi2c);

    // won't reach here without the update loop exiting
    hx711_i2c_slave_close(&hxi2c);

    hx711_close(&hx);

    printf("Exiting\n");

    return EXIT_SUCCESS;

}