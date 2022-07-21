// MIT License
// 
// Copyright (c) 2022 Daniel Robertson
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
#include <tusb.h>
#include "pico/stdio.h"
#include "../include/hx711.h"
#include "../include/hx711_noblock.h"
#include "hx711_noblock.pio.h"

int main(void) {

    stdio_init_all();

    //wait for serial connection
    while(!tud_cdc_connected()) {
        sleep_ms(10);
    }

    // PINOUT REFERENCE: https://learn.adafruit.com/assets/99339
    const uint clkPin = 14; // GP14, PAD19
    const uint datPin = 15; // GP15, PAD20

    hx711_t hx;
    
    hx711_init(
        &hx,
        clkPin,
        datPin,
        pio0,
        &hx711_noblock_program,
        &hx711_noblock_program_init);

    while(true) {

        hx711_set_power(&hx, hx711_pwr_up);
        hx711_set_gain(&hx, hx711_gain_128);
        hx711_wait_settle(hx711_rate_80);

        for(int i = 0; i < 1; ++i) {
            printf("%li\n", hx711_get_value(&hx));
        }

        hx711_set_power(&hx, hx711_pwr_down);
        hx711_wait_power_down();

    }

    return EXIT_SUCCESS;

}
