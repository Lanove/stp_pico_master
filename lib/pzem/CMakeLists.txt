cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(PZEM)

file(GLOB FILES ./*.cpp ./*.h)
add_library(PZEM STATIC ${FILES})

target_link_libraries(PZEM PRIVATE
    pico_stdlib
    ModbusMaster
    )
# Following two libraries must be PUBLIC, idk
target_link_libraries(PZEM PUBLIC 
)

target_include_directories(PZEM PUBLIC ./)