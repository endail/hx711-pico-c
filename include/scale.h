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

#ifndef _SCALE_H_
#define _SCALE_H_

#include "hx711.h"
#include "mass.h"

typedef enum {
    strategy_type_samples,
    strategy_type_time
} strategy_type_t;

typedef enum {
    read_type_median,
    read_type_average
} read_type_t;

typedef struct {
    strategy_type_t strat_type;
    read_type_t read_type;
    size_t samples;
    uint64_t timeout;
} scale_options_t;

typedef struct {

    int32_t offset;
    int32_t ref_unit;
    mass_unit_t unit;

    hx711_t* _hx;

} scale_t;

void scale_init(
    scale_t* const sc,
    hx711_t* const hx,
    const int32_t offset,
    const int32_t ref_unit,
    const mass_unit_t unit);

void scale_normalise(
    const scale_t* const sc,
    const double* const raw,
    double* const normalised);

void scale_get_values_samples(
    scale_t* const sc,
    int32_t* const arr,
    const size_t len);

void scale_get_values_timeout(
    scale_t* const sc,
    int32_t* arr,
    size_t* len,
    const uint64_t timeout);

void scale_read(
    scale_t* const sc,
    double* const val,
    const scale_options_t* const opt);

void scale_zero(
    scale_t* const sc,
    const scale_options_t* const opt);

void scale_weight(
    scale_t* const sc,
    mass_t* const m,
    const scale_options_t* const opt);

#endif
