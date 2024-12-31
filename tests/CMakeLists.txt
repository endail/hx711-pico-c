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

add_compile_options(
        -Wall
        -Wextra
        -Werror
        -Wfatal-errors
        -Wfloat-equal
        -Wunreachable-code
        -Wno-unused-function
        -Wno-maybe-uninitialized
        -Wno-ignored-qualifiers
        -Wno-stringop-overflow          # rp2040_usb.c:61:3
        -Wno-array-bounds               # rp2040_usb.c:61:3
        )

add_executable(main
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        )

add_executable(i2c_master
        ${CMAKE_CURRENT_LIST_DIR}/i2c_master.c
        )

add_executable(i2c_slave
        ${CMAKE_CURRENT_LIST_DIR}/i2c_slave.c
        )

add_executable(spi_master
        ${CMAKE_CURRENT_LIST_DIR}/spi_master.c
        )

add_executable(spi_slave
        ${CMAKE_CURRENT_LIST_DIR}/spi_slave.c
        )

target_link_libraries(main
        hx711-pico-c
        pico_stdlib
        pico_stdio
        )

target_link_libraries(i2c_master
        hx711-pico-c
        pico_stdlib
        pico_stdio
        )

target_link_libraries(i2c_slave
        hx711-pico-c
        pico_stdlib
        pico_stdio
        )

target_link_libraries(spi_master
        hx711-pico-c
        pico_stdlib
        pico_stdio
        )

target_link_libraries(spi_slave
        hx711-pico-c
        pico_stdlib
        pico_stdio
        )

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)
pico_add_extra_outputs(main)

pico_enable_stdio_usb(i2c_master 1)
pico_enable_stdio_uart(i2c_master 1)
pico_add_extra_outputs(i2c_master)

pico_enable_stdio_usb(i2c_slave 1)
pico_enable_stdio_uart(i2c_slave 1)
pico_add_extra_outputs(i2c_slave)

pico_enable_stdio_usb(spi_master 1)
pico_enable_stdio_uart(spi_master 1)
pico_add_extra_outputs(spi_master)

pico_enable_stdio_usb(spi_slave 1)
pico_enable_stdio_uart(spi_slave 1)
pico_add_extra_outputs(spi_slave)
