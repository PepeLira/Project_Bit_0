cmake_minimum_required(VERSION 3.13...3.27)

include(3rd_party/pico-sdk/pico_sdk_init.cmake)

project(i2c_keyboard C CXX ASM)

pico_sdk_init()

# Core timing services
set(CORE_SOURCES
    src/core/tick.c
)

# Hardware abstraction layer
set(HARDWARE_SOURCES
    src/hardware/button.c
    src/hardware/led.c
    src/hardware/i2c_slave.c
    src/hardware/power_latch.c
)

# Input processing modules
set(INPUT_SOURCES
    src/input/matrix_scanner.c
    src/input/fn_keys.c
    src/input/modifier_manager.c
    src/input/digital_mouse.c
    src/input/key_fifo.c
    src/input/switch_tracker.c
)

# Application layer
set(APP_SOURCES
    src/app/main.c
    src/app/led_controller.c
)

add_executable(i2c_keyboard
    ${CORE_SOURCES}
    ${HARDWARE_SOURCES}
    ${INPUT_SOURCES}
    ${APP_SOURCES}
)

pico_generate_pio_header(i2c_keyboard ${CMAKE_CURRENT_LIST_DIR}/src/hardware/ws2812.pio)

target_include_directories(i2c_keyboard PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/core
    ${CMAKE_CURRENT_LIST_DIR}/src/hardware
    ${CMAKE_CURRENT_LIST_DIR}/src/input
    ${CMAKE_CURRENT_LIST_DIR}/src/app
    ${CMAKE_CURRENT_LIST_DIR}/src/config
)

target_link_libraries(i2c_keyboard pico_stdlib hardware_pio hardware_timer hardware_i2c)

pico_add_extra_outputs(i2c_keyboard)

set_property(TARGET i2c_keyboard PROPERTY C_STANDARD 11)

add_library(switch_logic STATIC src/input/switch_tracker.c)
target_include_directories(switch_logic PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/input
)
set_property(TARGET switch_logic PROPERTY C_STANDARD 11)
