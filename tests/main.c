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
#include "pico/stdio.h"
#include "../include/hx711.h"
#include "../include/hx711_noblock.pio.h"
#include "../include/hx711_multi.h"
#include "../include/hx711_multi_reader.pio.h"

int main(void) {

    stdio_init_all();

    // SET THESE TO THE GPIO PINS CONNECTED TO THE
    // HX711's CLOCK AND DATA PINS
    // PINOUT REFERENCE: https://learn.adafruit.com/assets/99339
    const uint clkPin = 14; // GP14, PAD19
    const uint datPin = 15; // GP15, PAD20

    hx711_t hx;

    //1. init the hx711 struct
    hx711_init(
        &hx,
        clkPin,
        datPin,
        pio0,
        &hx711_noblock_program,
        &hx711_noblock_program_init);

    //2. power up the hx711 and use a gain of 128
    hx711_power_up(&hx, hx711_gain_128);

    //3. This step is optional. Only do this if you want to
    //change the gain AND save it to the HX711 chip
    //
    //hx711_set_gain(&hx, hx711_gain_64);
    //hx711_power_down(&hx);
    //hx711_wait_power_down();
    //hx711_power_up(&hx, hx711_gain_64);

    //4. pause to allow the readings to settle based on the
    //sample rate of the chip
    hx711_wait_settle(hx711_rate_80);

    //at this point, the hx711 can reliably produce values
    //with hx711_get_value or hx711_get_value_timeout
    printf("%li\n", hx711_get_value(&hx));

    hx711_close(&hx);


    hx711_multi_t hxm;
    const uint chips = 1;

    hx711_multi_init(
        &hxm,
        clkPin,
        datPin,
        chips,
        pio0,
        hx711_multi_pio_init,
        hx711_multi_awaiter_program,
        hx711_multi_awaiter_program_init,
        hx711_multi_reader_program,
        hx711_multi_reader_program_init);

    hx711_multi_power_up(&hxm, hx711_gain_128);

    int32_t arr[chips];

    hx711_multi_get_values(&hxm, arr);

    for(uint i = 0; i < chips; ++i) {
        printf("hx711_multi_t chip %li: %li\n", i, arr[i]);
    }

    return EXIT_SUCCESS;

}
