cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(xpt2046)

file(GLOB FILES ./*.cpp ./*.h)
add_library(xpt2046 STATIC ${FILES})

target_link_libraries(xpt2046 PRIVATE
    pico_stdlib
    ili9486_drivers
    )
# Following two libraries must be PUBLIC, idk
target_link_libraries(xpt2046 PUBLIC 
    hardware_spi
)

# add_subdirectory(../../ili9486_drivers ${CMAKE_CURRENT_BINARY_DIR}/ili9486_drivers)
target_include_directories(xpt2046 PUBLIC ./)