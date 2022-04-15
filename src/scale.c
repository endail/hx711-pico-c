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

#include "../include/scale.h"
#include "../include/util.h"
#include <math.h>
#include <stdlib.h>

void scale_init(
    scale_t* const sc,
    hx711_t* const hx,
    const int32_t offset,
    const int32_t ref_unit,
    const mass_unit_t unit) {
        sc->_hx = hx;
        sc->offset = offset;
        sc->ref_unit = ref_unit;
        sc->unit = unit;
}

void scale_normalise(
    const scale_t* const sc,
    const double* const raw,
    double* const normalised) {
        *normalised = (*raw - sc->offset) / sc->ref_unit;
}

void scale_get_values_samples(
    scale_t* const sc,
    int32_t* const arr,
    const size_t len) {

        for(size_t i = 0; i < len; ++i) {
            arr[i] = hx711_get_value(sc->_hx);
        }

}

void scale_get_values_timeout(
    scale_t* const sc,
    int32_t* arr,
    size_t* len,
    const uint64_t timeout) {

        size_t curLen = 0;
        int32_t* curArr = NULL;

        const absolute_time_t endTime = delayed_by_us(
            get_absolute_time(),
            timeout);

        while(!time_reached(endTime)) {
            curArr = (int32_t*)realloc(curArr, ++curLen * sizeof(int32_t));
            curArr[curLen - 1] = hx711_get_value(sc->_hx);
        }

        arr = curArr;
        *len = curLen;

}

void scale_read(
    scale_t* const sc,
    double* const val,
    const scale_options_t* const opt) {

        int32_t* arr;
        size_t len;

        switch(opt->strat_type) {
        case strategy_type_samples:
            arr = (int32_t*)malloc(opt->samples * sizeof(int32_t));
            scale_get_values_samples(sc, arr, opt->samples);
            len = opt->samples;
            break;
        case strategy_type_time:
            scale_get_values_timeout(sc, arr, &len, opt->timeout);
            break;
        default:
            return;
        }
        
        if(len == 0) {
            //return an error
            return;
        }

        switch(opt->read_type) {
        case read_type_median:
            util_median(arr, len, val);
            break;
        case read_type_average:
            util_average(arr, len, val);
            break;
        default:
            //error!
            break;
        }

        free(arr);

}

void scale_zero(
    scale_t* const sc,
    const scale_options_t* const opt) {

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
        double val;
        scale_read(sc, &val, opt);
        scale_normalise(sc, &val, &val);
        mass_set_value(m, sc->unit, &val);
}