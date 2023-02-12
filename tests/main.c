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
#include "../include/common.h"
#include "tusb.h"

int main(void) {

    stdio_init_all();

//    while (!tud_cdc_connected()) {
//        sleep_ms(1);
//    }

/*
    hx711_t hx;
    hx711_config_t config = HX711_DEFAULT_CONFIG;
    config.clock_pin = 14;
    config.data_pin = 15;
    const hx711_rate_t rate = hx711_rate_80; //or hx711_rate_80
    const hx711_gain_t gain = hx711_gain_128; //or hx711_gain_64 or hx711_gain_32

    // 1. Initialise
    hx711_init(&hx, &config);

    //2. Power up the hx711 and set gain on chip
    hx711_power_up(&hx, gain);

    //3. This step is optional. Only do this if you want to
    //change the gain AND save it to the HX711 chip
    //
    //hx711_set_gain(&hx, hx711_gain_64);
    //hx711_power_down(&hx);
    //hx711_wait_power_down();
    //hx711_power_up(&hx, hx711_gain_64);

    // 4. Wait for readings to settle
    hx711_wait_settle(rate);

    // 5. Read values
    // You can now...

    // wait (block) until a value is obtained
    printf("blocking value: %li\n", hx711_get_value(&hx));

    // or use a timeout
    int32_t val;
    const uint timeout = 250000; //microseconds
    if(hx711_get_value_timeout(&hx, &val, timeout)) {
        // value was obtained within the timeout period,
        // in this case within 250 milliseconds
        printf("timeout value: %li\n", val);
    }
    else {
        printf("value was not obtained within the timeout period\n");
    }

    // or see if there's a value, but don't block if there isn't one ready
    if(hx711_get_value_noblock(&hx, &val)) {
        printf("noblock value: %li\n", val);
    }
    else {
        printf("value was not present\n");
    }

    //6. Stop communication with HX711
    hx711_close(&hx);

    printf("Closed communication with single HX711 chip\n");
*/



    hx711_multi_t hxm;
    hx711_multi_config_t cfg = HX711_MULTI_DEFAULT_CONFIG;
    cfg.clock_pin = 14;
    cfg.data_pin_base = 15;
    cfg.chips_len = 1;
    const hx711_rate_t multi_rate = hx711_rate_80; //or hx711_rate_80
    const hx711_gain_t multi_gain = hx711_gain_128; //or hx711_gain_64 or hx711_gain_32

    // 1. initialise
    hx711_multi_init(&hxm, &cfg);

    // 2. Power up the HX711 chips and set gain on each chip
    hx711_multi_power_up(&hxm, multi_gain);

    //3. This step is optional. Only do this if you want to
    //change the gain AND save it to each HX711 chip
    //
    //hx711_multi_set_gain(&hxm, hx711_gain_64);
    //hx711_multi_power_down(&hxm);
    //hx711_wait_power_down();
    //hx711_multi_power_up(&hxm, hx711_gain_64);

    // 4. Wait for readings to settle
    hx711_wait_settle(multi_rate);

    // 5. Read values
    int32_t arr[cfg.chips_len];


    hx711_multi_async_request_t req;

    hx711_multi_async_get_request_defaults(&hxm, &req);
    hx711_multi_async_open(&hxm, &req);

    for(uint loops = 0; loops < 5000; ++loops) {

        //sleep_ms(rand() % 500);

        hx711_multi_async_start(&req);

        //sleep_ms(rand() % 500);

        while(!hx711_multi_async_is_done(&req)) {
            // do other stuff...
            tight_loop_contents();
        }

        //sleep_ms(rand() % 500);

        hx711_multi_async_get_values(&req, arr);

        // wait (block) until a values are read
//        hx711_multi_get_values(&hxm, arr);

        // or use a timeout
        
        //printf("%lu\n", hx711_multi_sync_state(&hxm));

        //if(!hx711_multi_get_values_timeout(&hxm, arr, 250000)) {
        //    printf("Failed to obtain values within timeout\n");
        //    continue;
        //}

        // then print the value from each chip
        // the first value in the array is from the HX711
        // connected to the first configured data pin and
        // so on
        for(uint i = 0; i < cfg.chips_len; ++i) {
            printf("hx711_multi_t chip %i: %li\n", i, arr[i]);
        }

    }

    hx711_multi_async_close(&hxm, &req);

    // 6. Stop communication with all HX711 chips
    hx711_multi_close(&hxm);

    printf("Closed communication with multiple HX711 chips\n");
    

    while(1);

    return EXIT_SUCCESS;

}
