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
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/mass.h"

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

        //if less than the epsilon, then it's ~0
        if(!(fabs(f) < DBL_EPSILON)) { 
            d = (int)fmax(0, ceil(1 - log10(fabs(f))));
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

        if(fromUnit == toUnit) {
            *toAmount = *fromAmount;
        }
        else if(toUnit == mass_ug) {
            *toAmount = *fromAmount * *mass_unit_to_ratio(fromUnit);
        }
        else if(fromUnit == mass_ug) {
            *toAmount = *fromAmount / *mass_unit_to_ratio(toUnit);
        }
        else {
            mass_convert(fromAmount, toAmount, toUnit, mass_ug);
        }

}