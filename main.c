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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "../include/hx711.h"
#include "../include/scale.h"
#include "hx711_noblock.pio.h"

int main() {

    stdio_init_all();

    const uint clkPin = 14;
    const uint datPin = 15;
    const int32_t refUnit = -440;
    const int32_t offset = -365858;
    const mass_unit_t unit = mass_g;

    hx711_t hx;
    scale_t sc;
    scale_options_t opt = SCALE_DEFAULT_OPTIONS;
    mass_t mass;
    char buff[MASS_TO_STRING_BUFF_SIZE];

    hx711_init(
        &hx,
        clkPin,
        datPin,
        pio0,
        &hx711_noblock_program,
        &hx711_noblock_program_init);

    //set gain and reset
    //powering down and then powering back up after setting
    //the gain saves the gain to the HX711
    hx711_set_gain(&hx, hx711_gain_128);
    hx711_set_power(&hx, hx711_pwr_down);
    sleep_us(HX711_POWER_DOWN_TIMEOUT);
    hx711_set_power(&hx, hx711_pwr_up);
    sleep_ms(hx711_get_settling_time(hx711_rate_80));

    scale_init(&sc, &hx, offset, refUnit, unit);

    //spend 10 seconds obtaining as many samples as possible
    opt.strat = strategy_type_time;
    opt.timeout = 10000000;

    if(scale_zero(&sc, &opt)) {
        printf("Scale zeroed successfully\n");
    }
    else {
        printf("Scale failed to zero\n");
    }

    opt.timeout = 250000;

    mass_t max;
    mass_t min;

    mass_init(&max, mass_g, 0);
    mass_init(&min, mass_g, 0);

    for(;;) {

        memset(buff, 0, MASS_TO_STRING_BUFF_SIZE);

        if(scale_weight(&sc, &mass, &opt)) {

            if(mass_lt(&mass, &min)) {
                min = mass;
            }

            if(mass_gt(&mass, &max)) {
                max = mass;
            }

            mass_to_string(&mass, buff);
            printf("%s", buff);

            mass_to_string(&min, buff);
            printf(" min: %s", buff);

            mass_to_string(&max, buff);
            printf(" max: %s\n", buff);

        }
        else {
            printf("Failed to read weight\n");
        }

    }

    return EXIT_SUCCESS;

}
