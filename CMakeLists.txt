# MIT License
# 
# Copyright (c) 2022 Daniel Robertson
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(hx711test C CXX ASM)
set(CMAKE_C_STANDARD 11)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

add_executable(main
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711.c
        ${CMAKE_CURRENT_LIST_DIR}/src/mass.c
        ${CMAKE_CURRENT_LIST_DIR}/src/scale.c
        ${CMAKE_CURRENT_LIST_DIR}/src/util.c
        )

pico_generate_pio_header(main
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711_noblock.pio
        )

target_link_libraries(main
        hardware_clocks
        hardware_gpio
        hardware_pio
        hardware_timer
        pico_divider
        pico_double
        pico_float
        pico_int64_ops
        pico_malloc
        pico_mem_ops
        pico_stdlib
        pico_sync
        pico_time
        )

pico_add_extra_outputs(main)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

add_compile_options(-Wall
        -Wno-format
        -Wno-unused-function
        -Wno-maybe-uninitialized
        )
