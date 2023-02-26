// MIT License
// 
// Copyright (c) 2023 Daniel Robertson
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

#define PRINT_ARR(arr, len) \
    do { \
        for(size_t i = 0; i < len; ++i) { \
            printf("hx711_multi_t chip %i: %li\n", i, arr[i]); \
        } \
    } while(0)

int main(void) {

    stdio_init_all();

    while (!tud_cdc_connected()) {
        sleep_ms(1);
    }

#if 1
    hx711_config_t hxcfg;
    hx711_get_default_config(&hxcfg);

    hxcfg.clock_pin = 14;
    hxcfg.data_pin = 15;

    hx711_t hx;

    // 1. Initialise
    hx711_init(&hx, &hxcfg);

    //2. Power up the hx711 and set gain on chip
    hx711_power_up(&hx, hx711_gain_128);

    //3. This step is optional. Only do this if you want to
    //change the gain AND save it to the HX711 chip
    //
    //hx711_set_gain(&hx, hx711_gain_64);
    //hx711_power_down(&hx);
    //hx711_wait_power_down();
    //hx711_power_up(&hx, hx711_gain_64);

    // 4. Wait for readings to settle
    hx711_wait_settle(hx711_rate_80);

    // 5. Read values
    // You can now...

    // wait (block) until a value is obtained
    // cppcheck-suppress invalidPrintfArgType_sint
    printf("blocking value: %li\n", hx711_get_value(&hx));

    // or use a timeout
    int32_t val;
    const uint timeout = 250000; //microseconds
    if(hx711_get_value_timeout(&hx, &val, timeout)) {
        // value was obtained within the timeout period,
        // in this case within 250 milliseconds
        // cppcheck-suppress invalidPrintfArgType_sint
        printf("timeout value: %li\n", val);
    }
    else {
        printf("value was not obtained within the timeout period\n");
    }

    // or see if there's a value, but don't block if there isn't one ready
    if(hx711_get_value_noblock(&hx, &val)) {
        // cppcheck-suppress invalidPrintfArgType_sint
        printf("noblock value: %li\n", val);
    }
    else {
        printf("value was not present\n");
    }

    //6. Stop communication with HX711
    hx711_close(&hx);

    printf("Closed communication with single HX711 chip\n");
#endif

#if 1
    hx711_multi_config_t hxmcfg;
    hx711_multi_get_default_config(&hxmcfg);
    hxmcfg.clock_pin = 14;
    hxmcfg.data_pin_base = 15;
    hxmcfg.chips_len = 1;
    hxmcfg.pio_irq_index = 1;
    hxmcfg.dma_irq_index = 1;

    hx711_multi_t hxm;

    // 1. initialise
    hx711_multi_init(&hxm, &hxmcfg);

    // 2. Power up the HX711 chips and set gain on each chip
    hx711_multi_power_up(&hxm, hx711_gain_128);

    //3. This step is optional. Only do this if you want to
    //change the gain AND save it to each HX711 chip
    //
    //hx711_multi_set_gain(&hxm, hx711_gain_64);
    //hx711_multi_power_down(&hxm);
    //hx711_wait_power_down();
    //hx711_multi_power_up(&hxm, hx711_gain_64);

    // 4. Wait for readings to settle
    hx711_wait_settle(hx711_rate_80);

    // 5. Read values
    int32_t arr[hxmcfg.chips_len];

    // wait (block) until a values are read
    hx711_multi_get_values(&hxm, arr);
    PRINT_ARR(arr, hxmcfg.chips_len);

    // or use a timeout
    if(hx711_multi_get_values_timeout(&hxm, arr, 250000)) {
        PRINT_ARR(arr, hxmcfg.chips_len);
    }
    else {
        printf("Failed to obtain values within timeout\n");
    }

    hx711_multi_async_start(&hxm);

    while(!hx711_multi_async_done(&hxm)) {
        // do other stuff...
        tight_loop_contents();
    }

    hx711_multi_async_get_values(&hxm, arr);
    PRINT_ARR(arr, hxmcfg.chips_len);

    // 6. Stop communication with all HX711 chips
    hx711_multi_close(&hxm);

    printf("Closed communication with multiple HX711 chips\n");
#endif

    while(1);

    return EXIT_SUCCESS;

}
