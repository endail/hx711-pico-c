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

#include "../include/util.h"
#include <stdlib.h>

void util_average(
    const int32_t* const arr,
    const size_t len,
    double* const avg) {

        assert(arr != NULL);
        assert(len > 0);
        assert(avg != NULL);

        long long int total = 0;

        for(size_t i = 0; i < len; ++i) {
            total += arr[i];
        }

        *avg = (double)total / len;

}

void util_median(
    int32_t* const arr,
    const size_t len,
    double* const med) {

        assert(arr != NULL);
        assert(len > 0);
        assert(med != NULL);

        qsort(
            arr,
            len,
            sizeof(int32_t),
            util__median_compare_func);

        /**
         * If the number of elements is even, the median is
         * the average of the middle two elements. Otherwise
         * it is the middle element.
         */
        if(len % 2 == 0) {
            *med = (arr[(len / 2) - 1] + arr[len / 2]) / 2.0;
        }
        else {
            *med = (double)arr[(len / 2) + 1];
        }

}
