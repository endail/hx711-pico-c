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
#include <stdio.h>
#include <stdlib.h>
#include "../include/mass.h"

const double MASS_RATIOS[] = {
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

const char* const MASS_NAMES[] = {
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

const uint8_t MASS_TO_STRING_BUFF_SIZE = 64;

void mass_get_value(
    const mass_t* const m,
    double* const val) {

        assert(m != NULL);
        assert(val != NULL);

        mass_convert(&m->ug, val, mass_ug, m->unit);

}

void mass_set_value(
    mass_t* const m,
    const mass_unit_t unit,
    const double* const val) {

        assert(m != NULL);
        assert(val != NULL);

        mass_convert(val, &m->ug, unit, mass_ug);
        m->unit = unit;

}

int mass_to_string(
    const mass_t* const m,
    char* const buff) {

        assert(m != NULL);
        assert(buff != NULL);

        double n; //value
        mass_convert(&m->ug, &n, mass_ug, m->unit);
        double i; //int part
        const double f = modf(n, &i); //frac part
        int d = 0; //decimal count

        if(f != 0) {
            d = (int)fmax(0, (1 - log10(fabs(f))));
        }

        return snprintf(
            buff,
            MASS_TO_STRING_BUFF_SIZE,
            "%01.*f %s",
            d,
            n,
            mass_unit_to_string(m->unit));

}

void mass_convert(
    const double* const fromAmount,
    double* const toAmount,
    const mass_unit_t fromUnit,
    const mass_unit_t toUnit) {

        assert(fromAmount != NULL);
        assert(toAmount != NULL);

        if(toUnit == mass_ug) {
            *toAmount = *fromAmount * *mass_unit_to_ratio(fromUnit);
            return;
        }

        if(fromUnit == mass_ug) {
            *toAmount = *fromAmount / *mass_unit_to_ratio(toUnit);
            return;
        }

        if(fromUnit == toUnit) {
            *toAmount = *fromAmount;
            return;
        }

        mass_convert(fromAmount, toAmount, toUnit, mass_ug);

}