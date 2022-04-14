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

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "../include/hx711.h"
#include "hx711_noblock.pio.h"

int main() {

    stdio_init_all();

    hx711_t hx;

    hx711_init(
        &hx,
        14, //gpio 14
        15, //gpio 15
        pio0,
        &hx711_noblock_program,
        &hx711_noblock_program_init);

    hx711_set_gain(&hx, gain_128);

    //sleep_ms(400); //settling time @ 10Hz
    sleep_ms(50); //settling time @ 80Hz

    while(true) {
        printf("%i\n", hx711_get_value_fast(&hx));
    }

    return 0;

}
