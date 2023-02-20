// MIT License
// 
// Copyright (c) 2023 Daniel Robertson
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

#ifndef COMMON_H_D032FD58_DAFE_4AE1_8E48_54A388BFECE4
#define COMMON_H_D032FD58_DAFE_4AE1_8E48_54A388BFECE4

#include "hx711.h"
#include "hx711_multi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const hx711_config_t HX711__DEFAULT_CONFIG;
extern const hx711_multi_config_t HX711__MULTI_DEFAULT_CONFIG;

void hx711_get_default_config(hx711_config_t* const cfg);
void hx711_multi_get_default_config(hx711_multi_config_t* const cfg);

#ifdef __cplusplus
}
#endif

#endif
