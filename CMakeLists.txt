# MIT License
# 
# Copyright (c) 2023 Daniel Robertson
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

# this must go above project()!
include(pico_sdk_import.cmake)

project(hx711-pico-c
        VERSION 2.0.2
        DESCRIPTION "Implementation of HX711 use via RP2040's state machine"
        HOMEPAGE_URL "https://github.com/endail/hx711-pico-c"
        LANGUAGES C CXX ASM
        )

# only do this when this project is the one being built
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

        set(CMAKE_C_STANDARD 11)
        set_property(GLOBAL PROPERTY USE_FOLDERS ON)

        include(CTest)

        find_package(Doxygen)

        if(Doxygen_FOUND)
                add_subdirectory(docs)
        else()
                message(STATUS "Doxygen not found, not building docs")
        endif()

        # init the pico sdk only when testing
        if(BUILD_TESTING)
                # this must go below project()
                pico_sdk_init()
        endif()

endif()

# check for minimum pico sdk version
if(PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
        message(FATAL_ERROR "hx711-pico-c requires Raspberry Pi Pico SDK version 1.5.0 (or later). Your version is ${PICO_SDK_VERSION_STRING}")
endif()

add_library(hx711-pico-c INTERFACE)

#target_compile_definitions(hx711-pico-c INTERFACE
#        HX711_NO_MUTEX
#        )

target_link_libraries(hx711-pico-c INTERFACE
        hardware_clocks
        hardware_dma
        hardware_gpio
        hardware_irq
        hardware_pio
        hardware_timer
        pico_platform
        pico_sync
        pico_time
        )

target_sources(hx711-pico-c INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711_multi.c
        ${CMAKE_CURRENT_LIST_DIR}/src/common.c
        ${CMAKE_CURRENT_LIST_DIR}/src/util.c
        )

# when running the tests in this project, build the main test exe
# side effect is that no tests are run, but we don't care; just
# want to build the test program
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
        add_subdirectory(tests)
endif()
