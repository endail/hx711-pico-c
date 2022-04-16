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

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "pico/malloc.h"
#include "pico/platform.h"
#include "pico/time.h"
#include "../include/scale.h"
#include "../include/util.h"

const scale_options_t SCALE_DEFAULT_OPTIONS = {
    .strat_type = strategy_type_samples,
    .read_type = read_type_median,
    .samples = 3, //3 samples
    .timeout = 1000000 //1 second
};

void scale_init(
    scale_t* const sc,
    hx711_t* const hx,
    const int32_t offset,
    const int32_t ref_unit,
    const mass_unit_t unit) {
        //TODO: init a mutex here?
        sc->_hx = hx;
        sc->offset = offset;
        sc->ref_unit = ref_unit;
        sc->unit = unit;
}

void scale_normalise(
    const scale_t* const sc,
    const double* const raw,
    double* const normalised) {

        assert(sc != NULL);
        assert(raw != NULL);
        assert(normalised != NULL);

        *normalised = (*raw - sc->offset) / sc->ref_unit;

}

void scale_get_values_samples(
    scale_t* const sc,
    int32_t** const arr,
    const size_t len) {

        assert(sc != NULL);
        assert(arr != NULL);

        //TODO: error checking
        *arr = malloc(__fast_mul(len, sizeof(int32_t)));

        for(size_t i = 0; i < len; ++i) {
            (*arr)[i] = hx711_get_value(sc->_hx);
        }

}

void scale_get_values_timeout(
    scale_t* const sc,
    int32_t** arr,
    size_t* len,
    const uint64_t* const timeout) {

        assert(sc != NULL);
        assert(arr != NULL);
        assert(len != NULL);
        assert(timeout != NULL);

        const absolute_time_t endTime = make_timeout_time_us(*timeout);

        while(!time_reached(endTime)) {
            ++(*len);
            //TODO: error checking
            *arr = realloc(*arr, __fast_mul(*len, sizeof(int32_t)));
            //hx711_get_value will block, so no busy-waiting here
            (*arr)[(*len) - 1] = hx711_get_value(sc->_hx);
        }

}

void scale_read(
    scale_t* const sc,
    double* const val,
    const scale_options_t* const opt) {

        assert(sc != NULL);
        assert(val != NULL);
        assert(opt != NULL);

        int32_t* arr = NULL;
        size_t len = 0;

        switch(opt->strat_type) {
        case strategy_type_time:
            scale_get_values_timeout(sc, &arr, &len, &opt->timeout);
            break;

        case strategy_type_samples:
        default:
            scale_get_values_samples(sc, &arr, opt->samples);
            len = opt->samples;
            break;

        }
        
        if(len == 0) {
            //TODO: return an error?
            free(arr);
            return;
        }

        switch(opt->read_type) {
        case read_type_average:
            util_average(arr, len, val);
            break;
        case read_type_median:
        default:
            util_median(arr, len, val);
            break;
        }

        free(arr);

}

void scale_zero(
    scale_t* const sc,
    const scale_options_t* const opt) {

        assert(sc != NULL);
        assert(opt != NULL);

        double val;
        const int32_t refBackup = sc->ref_unit;

        sc->ref_unit = 1;
        scale_read(sc, &val, opt);
        sc->offset = (int32_t)round(val);
        sc->ref_unit = refBackup;

}

void scale_weight(
    scale_t* const sc,
    mass_t* const m,
    const scale_options_t* const opt) {

        assert(sc != NULL);
        assert(m != NULL);
        assert(opt != NULL);

        double val;

        scale_read(sc, &val, opt);
        scale_normalise(sc, &val, &val);
        mass_set_value(m, sc->unit, &val);

}