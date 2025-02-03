cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(rs485_slaves)

file(GLOB FILES ./*.cpp ./*.h)
add_library(rs485_slaves STATIC ${FILES})

target_link_libraries(rs485_slaves PRIVATE
    pico_stdlib
    ModbusMaster
    )
# Following two libraries must be PUBLIC, idk
target_link_libraries(rs485_slaves PUBLIC 
)

target_include_directories(rs485_slaves PUBLIC ./)