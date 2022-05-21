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

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

static const double MASS_RATIOS[] = {
    1.0,
    1000.0,
    1000000.0,
    1000000000.0,
    1000000000000.0,
    1016046908800.0,
    907184740000.0,
    6350293180.0,
    453592370.0,
    28349523.125
};

static const char* const MASS_NAMES[] = {
    "μg",
    "mg",
    "g",
    "kg",
    "ton",
    "ton (IMP)",
    "ton (US)",
    "st",
    "lb",
    "oz"
};

static const size_t MASS_TO_STRING_BUFF_SIZE = 64;

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

static inline const char* const mass_unit_to_string(const mass_unit_t u) {
    return MASS_NAMES[(uint)u];
}

static inline const double* const mass_unit_to_ratio(const mass_unit_t u) {
    return &MASS_RATIOS[(uint)u];
}

typedef struct {
    double ug;
    mass_unit_t unit;
} mass_t;

void mass_convert(
    const double* const fromAmount,
    double* const toAmount,
    const mass_unit_t fromUnit,
    const mass_unit_t toUnit);

static inline void mass_init(
    mass_t* const m,
    const mass_unit_t unit,
    const double val) {

        assert(m != NULL);

        mass_convert(&val, &m->ug, unit, mass_ug);
        m->unit = unit;

}

static inline void mass_get_value(
    const mass_t* const m,
    double* const val) {

        assert(m != NULL);
        assert(val != NULL);

        mass_convert(&m->ug, val, mass_ug, m->unit);

}

static inline void mass_add(
    const mass_t* const lhs,
    const mass_t* const rhs,
    mass_t* const res) {

        assert(lhs != NULL);
        assert(rhs != NULL);
        assert(res != NULL);

        res->ug = lhs->ug + rhs->ug;
        res->unit = lhs->unit;

}

static inline void mass_sub(
    const mass_t* const lhs,
    const mass_t* const rhs,
    mass_t* const res) {

        assert(lhs != NULL);
        assert(rhs != NULL);
        assert(res != NULL);

        res->ug = lhs->ug - rhs->ug;
        res->unit = lhs->unit;

}

static inline void mass_mul(
    const mass_t* const lhs,
    const mass_t* const rhs,
    mass_t* const res) {

        assert(lhs != NULL);
        assert(rhs != NULL);
        assert(res != NULL);

        res->ug = lhs->ug * rhs->ug;
        res->unit = lhs->unit;

}

static inline bool mass_div(
    const mass_t* const lhs,
    const mass_t* const rhs,
    mass_t* const res) {

        assert(lhs != NULL);
        assert(rhs != NULL);
        assert(res != NULL);

        //if ~0; protect against div / 0
        if(fabs(lhs->ug) < DBL_EPSILON) {
            return false;
        }

        res->ug = lhs->ug / rhs->ug;
        res->unit = lhs->unit;
        return true;

}

static inline void mass_addeq(
    mass_t* const self,
    const mass_t* const rhs) {
        mass_add(self, rhs, self);
}

static inline void mass_subeq(
    mass_t* const self,
    const mass_t* const rhs) {
        mass_sub(self, rhs, self);
}

static inline void mass_muleq(
    mass_t* const self,
    const mass_t* const rhs) {
        mass_mul(self, rhs, self);
}

static inline bool mass_diveq(
    mass_t* const self,
    const mass_t* const rhs) {
        return mass_div(self, rhs, self);
}

static inline bool mass_eq(
    const mass_t* const lhs,
    const mass_t* const rhs) {

        assert(lhs != NULL);
        assert(rhs != NULL);

        //if ~==; if approx ~0
        return fabs(lhs->ug - rhs->ug) < DBL_EPSILON;

}

static inline bool mass_neq(
    const mass_t* const lhs,
    const mass_t* const rhs) {

        assert(lhs != NULL);
        assert(rhs != NULL);

        return !mass_eq(lhs, rhs);

}

static inline bool mass_lt(
    const mass_t* const lhs,
    const mass_t* const rhs) {

        assert(lhs != NULL);
        assert(rhs != NULL);

        return lhs->ug < rhs->ug;

}

static inline bool mass_gt(
    const mass_t* const lhs,
    const mass_t* const rhs) {

        assert(lhs != NULL);
        assert(rhs != NULL);

        return mass_lt(rhs, lhs);

}

static inline bool mass_lteq(
    const mass_t* const lhs,
    const mass_t* const rhs) {

        assert(lhs != NULL);
        assert(rhs != NULL);

        return !mass_gt(lhs, rhs);

}

static inline bool mass_gteq(
    const mass_t* const lhs,
    const mass_t* const rhs) {

        assert(lhs != NULL);
        assert(rhs != NULL);

        return !mass_lt(lhs, rhs);

}

/**
 * buff must be at least MASS_TO_STRING_BUFF_SIZE in length
 * return result of snprintf
 */
int mass_to_string(
    const mass_t* const m,
    char* const buff);

#ifdef __cplusplus
}
#endif

#endif
