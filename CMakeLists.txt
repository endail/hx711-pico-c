# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# this must go above project()!
include(pico_sdk_import.cmake)

project(hx711-pico-c
        VERSION 3.0.0
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
if(PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
        message(FATAL_ERROR "hx711-pico-c requires Raspberry Pi Pico SDK version 2.1.0 (or later). Your version is ${PICO_SDK_VERSION_STRING}")
endif()

add_library(hx711-pico-c INTERFACE)

#target_compile_definitions(hx711-pico-c INTERFACE
#        HX711_NO_MUTEX
#        )

target_link_libraries(hx711-pico-c INTERFACE
        hardware_clocks
        hardware_dma
        hardware_gpio
        hardware_i2c
        hardware_irq
        hardware_pio
        hardware_spi
        hardware_timer
        pico_divider
        pico_i2c_slave
        pico_mem_ops
        pico_platform
        pico_stdlib
        pico_sync
        pico_time
        )

target_sources(hx711-pico-c INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711_multi.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711_remote.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711_i2c_master.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711_i2c_slave.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711_spi_master.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hx711_spi_slave.c
        ${CMAKE_CURRENT_LIST_DIR}/src/util.c
        ${CMAKE_CURRENT_LIST_DIR}/src/common.c
        )

# when running the tests in this project, build the main test exe
# side effect is that no tests are run, but we don't care; just
# want to build the test program
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
        add_subdirectory(tests)
endif()