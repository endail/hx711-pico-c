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
#include <stdbool.h>
#include <stdlib.h>
#include "hardware/timer.h"
#include "pico/malloc.h"
#include "pico/platform.h"
#include "pico/time.h"
#include "../include/scale.h"
#include "../include/util.h"

void scale_init(
    scale_t* const sc,
    hx711_t* const hx,
    const int32_t offset,
    const int32_t ref_unit,
    const mass_unit_t unit) {

        assert(sc != NULL);
        assert(hx != NULL);
        assert(ref_unit != 0);

        sc->_hx = hx;
        sc->offset = offset;
        sc->ref_unit = ref_unit;
        sc->unit = unit;

}

bool scale_normalise(
    const scale_t* const sc,
    const double* const raw,
    double* const normalised) {

        assert(sc != NULL);
        assert(raw != NULL);
        assert(normalised != NULL);

        //protect against div / 0
        if(sc->ref_unit == 0) {
            return false;
        }

        *normalised = (*raw - sc->offset) / sc->ref_unit;
        return true;

}

bool scale_get_values_samples(
    scale_t* const sc,
    int32_t** const arr,
    const size_t len) {

        assert(sc != NULL);
        assert(sc->_hx != NULL);
        assert(arr != NULL);

        *arr = malloc(__fast_mul(len, sizeof(int32_t)));

        //if the allocation fails, return false
        if(*arr == NULL) {
            return false;
        }

        for(size_t i = 0; i < len; ++i) {
            (*arr)[i] = hx711_get_value(sc->_hx);
        }

        return true;

}

bool scale_get_values_timeout(
    scale_t* const sc,
    int32_t** const arr,
    size_t* const len,
    const absolute_time_t* const timeout) {

        assert(sc != NULL);
        assert(sc->_hx != NULL);
        assert(arr != NULL);
        assert(len != NULL);
        assert(timeout != NULL);
        assert(!is_nil_time(*timeout));

        int32_t val;
        int32_t* memblock;

        for(;;) {
            if(hx711_get_value_timeout(sc->_hx, timeout, &val)) {

                //new value available, so increase the counter
                ++(*len);

                //reallocate to increase the block size by 1
                memblock = realloc(
                    *arr,
                    __fast_mul(*len, sizeof(int32_t)));

                //if memory allocation fails, return false
                //existing *arr will still be allocated
                //but will be freed in caller function
                if(memblock == NULL) {
                    return false;
                }

                //move pointer of new block to *arr
                //then assign the value
                *arr = memblock;
                (*arr)[(*len) - 1] = val;

            }
            else {
                //timeout has been reached, so just
                //break out of the loop
                break;
            }
        }

        //no errors occurred, so return true
        return true;

}

bool scale_read(
    scale_t* const sc,
    double* const val,
    const scale_options_t* const opt) {

        assert(sc != NULL);
        assert(val != NULL);
        assert(opt != NULL);

        int32_t* arr = NULL;
        size_t len = 0;
        absolute_time_t timeout;
        bool ok = false; //assume error

        switch(opt->strat) {
        case strategy_type_time:
            timeout = make_timeout_time_us(opt->timeout);
            ok = scale_get_values_timeout(
                sc,
                &arr,
                &len,
                &timeout);
            break;

        case strategy_type_samples:
        default:
            len = opt->samples;
            ok = scale_get_values_samples(
                sc,
                &arr,
                opt->samples);
            break;
        }
        
        //if an error occurred or no samples obtained
        //deallocate any memory and then return false
        if(!ok || len == 0) {
            free(arr);
            return false;
        }

        switch(opt->read) {
        case read_type_average:
            util_average(arr, len, val);
            break;
        
        case read_type_median:
        default:
            util_median(arr, len, val);
            break;
        }

        //at this point, statistical func is done
        //so deallocate and return true
        free(arr);
        return true;

}

bool scale_zero(
    scale_t* const sc,
    const scale_options_t* const opt) {

        assert(sc != NULL);
        assert(opt != NULL);

        double val;
        const int32_t refBackup = sc->ref_unit;
        bool ok = false;

        sc->ref_unit = 1;
        
        if((ok = scale_read(sc, &val, opt))) {
            //only change the offset if the read
            //succeeded
            sc->offset = (int32_t)round(val);
        }

        sc->ref_unit = refBackup;

        return ok;

}

bool scale_weight(
    scale_t* const sc,
    mass_t* const m,
    const scale_options_t* const opt) {

        assert(sc != NULL);
        assert(m != NULL);
        assert(opt != NULL);

        double val;

        //if the read fails, return false
        if(!scale_read(sc, &val, opt)) {
            return false;
        }

        //if normalising the value fails, return false
        if(!scale_normalise(sc, &val, &val)) {
            return false;
        }

        mass_set_value(m, sc->unit, &val);

        return true;

}