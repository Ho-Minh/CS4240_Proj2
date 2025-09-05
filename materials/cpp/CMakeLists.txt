cmake_minimum_required(VERSION 3.15)
project(ircpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(ircpp
    src/ir_core.cpp
    src/ir_types.cpp
    src/ir_operands.cpp
    src/ir_reader.cpp
    src/ir_interpreter.cpp)

add_executable(demo examples/demo.cpp)
target_link_libraries(demo ircpp)


