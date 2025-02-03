cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(ModbusMaster)

file(GLOB FILES ./*.cpp ./*.h)
add_library(ModbusMaster STATIC ${FILES})

target_link_libraries(ModbusMaster PRIVATE
    pico_stdlib
    hardware_uart
    hardware_dma
    )
# Following two libraries must be PUBLIC, idk
target_link_libraries(ModbusMaster PUBLIC 
    hardware_uart
    hardware_dma
)

target_include_directories(ModbusMaster PUBLIC ./)