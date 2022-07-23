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

project(hx711-pico-c
        VERSION 1.0.0
        DESCRIPTION "Implementation of HX711 use via RP2040's state machine"
        HOMEPAGE_URL "https://github.com/endail/hx711-pico-c"
        LANGUAGES C CXX ASM
        )

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

        message("=================================================")

        set(CMAKE_C_STANDARD 11)
        set_property(GLOBAL PROPERTY USE_FOLDERS ON)

        #configure_file(hx711_pico_c_config.h.in
        #        "${CMAKE_CURRENT_LIST_DIR}/hx711_pico_c_config.h")

        include(CTest)

        find_package(Doxygen)
        if(Doxygen_FOUND)
                add_subdirectory(docs)
        else()
                message(STATUS "Doxygen not found, not building docs")
        endif()

endif()

add_library(hx711-pico-c INTERFACE)

target_link_libraries(hx711-pico-c INTERFACE
        hardware_clocks
        hardware_pio
        hardware_timer
        pico_sync
        pico_time
        )

target_sources(hx711-pico-c INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711.c
        )

#if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING) AND BUILD_TESTING)
        #enable_testing()
        #add_subdirectory(tests)
#endif()
