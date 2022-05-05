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
    const mass_unit_t unit = mass_g;
    const int32_t refUnit = -432;
    const int32_t offset = -367539;

    hx711_t hx;
    scale_t sc;
    scale_options_t opt = SCALE_DEFAULT_OPTIONS;
    mass_t mass;
    mass_t max;
    mass_t min;
    char buff[MASS_TO_STRING_BUFF_SIZE];

    //1. init the hx711 struct
    hx711_init(
        &hx,
        clkPin,
        datPin,
        pio0,
        &hx711_noblock_program,
        &hx711_noblock_program_init);

    //2. power up the hx711
    hx711_set_power(&hx, hx711_pwr_up);

    //3. [OPTIONAL] set gain and save it to the HX711
    //chip by powering down then back up
    hx711_set_gain(&hx, hx711_gain_128);
    hx711_set_power(&hx, hx711_pwr_down);
    sleep_us(HX711_POWER_DOWN_TIMEOUT);
    hx711_set_power(&hx, hx711_pwr_up);

    //4. pause to allow the readings to settle based on the
    //sample rate of the chip
    sleep_ms(hx711_get_settling_time(hx711_rate_80));

    //at this point, the hx711 can reliably produce values
    //with hx711_get_value or hx711_get_value_timeout

    //5. init the scale
    scale_init(&sc, &hx, unit, refUnit, offset);

    //6. spend 10 seconds obtaining as many samples as
    //possible to zero (aka. tare) the scale
    opt.strat = strategy_type_time;
    opt.timeout = 10000000;

    if(scale_zero(&sc, &opt)) {
        printf("Scale zeroed successfully\n");
    }
    else {
        printf("Scale failed to zero\n");
    }

    //7. change to spending 250 milliseconds obtaining
    //as many samples as possible
    opt.timeout = 250000;

    mass_init(&max, mass_g, 0);
    mass_init(&min, mass_g, 0);

    for(;;) {

        memset(buff, 0, MASS_TO_STRING_BUFF_SIZE);

        //8. obtain a mass from the scale
        if(scale_weight(&sc, &mass, &opt)) {

            //9. check if the newly obtained mass
            //is less than the existing minimum mass
            if(mass_lt(&mass, &min)) {
                min = mass;
            }

            //10. check if the newly obtained mass
            //is greater than the existing maximum mass
            if(mass_gt(&mass, &max)) {
                max = mass;
            }

            //11. display the newly obtained mass...
            mass_to_string(&mass, buff);
            printf("%s", buff);

            //...the current minimum mass...
            mass_to_string(&min, buff);
            printf(" min: %s", buff);

            //...and the current maximum mass
            mass_to_string(&max, buff);
            printf(" max: %s\n", buff);

        }
        else {
            printf("Failed to read weight\n");
        }

    }

    return EXIT_SUCCESS;

}
