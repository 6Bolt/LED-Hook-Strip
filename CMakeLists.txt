# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(LEDHookStrip C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(LEDHookStrip LEDHookStrip.c globals.h displayFunctions.h displayFunctions.c)

target_sources(LEDHookStrip PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/LEDHookStrip.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )


pico_set_program_name(LEDHookStrip "LEDHookStrip")
pico_set_program_version(LEDHookStrip "0.1")

# Generate PIO header
pico_generate_pio_header(LEDHookStrip ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(LEDHookStrip 1)
pico_enable_stdio_usb(LEDHookStrip 0)

# Add the standard library to the build
target_link_libraries(LEDHookStrip
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(LEDHookStrip PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)


# Add any user requested libraries
target_link_libraries(LEDHookStrip
        pico_stdlib
        hardware_pio
        pico_unique_id
        tinyusb_device
        tinyusb_board
        pico_multicore
        hardware_timer
        hardware_irq
        hardware_watchdog
)



pico_enable_stdio_usb(LEDHookStrip 1)


pico_add_extra_outputs(LEDHookStrip)



