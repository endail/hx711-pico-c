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

#ifndef _SCALE_H_4B64A868_05B1_4F4C_99CF_E75ED9BAED50
#define _SCALE_H_4B64A868_05B1_4F4C_99CF_E75ED9BAED50

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "pico/time.h"
#include "pico/types.h"
#include "hx711.h"
#include "mass.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    strategy_type_samples = 0,
    strategy_type_time
} strategy_type_t;

typedef enum {
    read_type_median = 0,
    read_type_average
} read_type_t;

typedef struct {
    strategy_type_t strat;
    read_type_t read;
    size_t samples;
    uint64_t timeout; //us
} scale_options_t;

static const scale_options_t SCALE_DEFAULT_OPTIONS = {
    .strat = strategy_type_samples,
    .read = read_type_median,
    .samples = 3, //3 samples
    .timeout = 1000000 //1 second
};

typedef struct {

    mass_unit_t unit;
    int32_t ref_unit;
    int32_t offset;

    hx711_t* _hx;

} scale_t;

void scale_init(
    scale_t* const sc,
    hx711_t* const hx,
    const mass_unit_t unit,
    const int32_t ref_unit,
    const int32_t offset);

bool scale_normalise(
    const scale_t* const sc,
    const double* const raw,
    double* const normalised);

bool scale_get_values_samples(
    scale_t* const sc,
    int32_t** const arr,
    const size_t len);

bool scale_get_values_timeout(
    scale_t* const sc,
    int32_t** const arr,
    size_t* const len,
    const absolute_time_t* const timeout);

bool scale_read(
    scale_t* const sc,
    double* const val,
    const scale_options_t* const opt);

bool scale_zero(
    scale_t* const sc,
    const scale_options_t* const opt);

bool scale_weight(
    scale_t* const sc,
    mass_t* const m,
    const scale_options_t* const opt);

#ifdef __cplusplus
}
#endif

#endif
