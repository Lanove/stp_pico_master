# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LV_CONF_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h
    CACHE STRING "" FORCE)
set(PICO_SDK_PATH "/home/kohigashi/git/pico-sdk")

# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(hmi_pico_stp C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

file(GLOB SRC_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
add_executable(hmi_pico_stp ${SRC_FILES})

pico_set_program_name(hmi_pico_stp "hmi_pico_stp")
pico_set_program_version(hmi_pico_stp "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(hmi_pico_stp 0)
pico_enable_stdio_usb(hmi_pico_stp 1)

# Add the standard library to the build
target_link_libraries(hmi_pico_stp
    pico_stdlib       
    hardware_clocks
    ili9486_drivers
    ClickEncoder
    xpt2046
    lvgl
    hardware_uart
    ModbusMaster
    rs485_slaves
    pico_multicore
)

# Add the standard include files to the build
target_include_directories(hmi_pico_stp PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${LVGL_ROOT_DIR}
)

message("LV_CONF_PATH: ${LV_CONF_PATH}")
add_subdirectory(lib/ili9486_drivers)
add_subdirectory(lib/lvgl)
add_subdirectory(lib/xpt2046)
add_subdirectory(lib/ClickEncoder)
add_subdirectory(lib/modbus_master)
add_subdirectory(lib/rs485_slaves)

pico_add_extra_outputs(hmi_pico_stp)

