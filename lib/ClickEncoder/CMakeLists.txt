cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(ClickEncoder)

file(GLOB FILES ./*.cpp ./*.h)
add_library(ClickEncoder STATIC ${FILES})

target_link_libraries(ClickEncoder PRIVATE
    pico_stdlib
    hardware_adc
    hardware_irq
    hardware_dma
    hardware_adc
    )
# Following two libraries must be PUBLIC, idk
target_link_libraries(ClickEncoder PUBLIC 
    hardware_spi
    hardware_dma
)

target_include_directories(ClickEncoder PUBLIC ./)