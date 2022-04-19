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

#ifndef _MASS_H_62063E22_421F_4578_BAD8_46AF5C66C4CF
#define _MASS_H_62063E22_421F_4578_BAD8_46AF5C66C4CF

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const double MASS_RATIOS[];
extern const char* const MASS_NAMES[];
const uint8_t MASS_TO_STRING_BUFF_SIZE;

/**
 * Enum values are mapped to MASS_RATIOS and MASS_NAMES
 */
typedef enum {
    mass_ug = 0,
    mass_mg,
    mass_g,
    mass_kg,
    mass_ton,
    mass_imp_ton,
    mass_us_ton,
    mass_st,
    mass_lb,
    mass_oz
} mass_unit_t;

const char* const mass_unit_to_string(const mass_unit_t u);
const double* const mass_unit_to_ratio(const mass_unit_t u);

typedef struct {
    double ug;
    mass_unit_t unit;
} mass_t;

void mass_get_value(
    const mass_t* const m,
    double* const val);

void mass_set_value(
    mass_t* const m,
    const mass_unit_t unit,
    const double* const val);


/**
 * buff must be at least MASS_TO_STRING_BUFF_SIZE in length
 * return result of snprintf
 */
int mass_to_string(
    const mass_t* const m,
    char* const buff);

void mass_convert(
    const double* const fromAmount,
    double* const toAmount,
    const mass_unit_t fromUnit,
    const mass_unit_t toUnit);

#ifdef __cplusplus
}
#endif

#endif
