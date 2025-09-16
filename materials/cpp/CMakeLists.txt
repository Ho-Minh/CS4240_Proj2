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
    src/ir_interpreter.cpp
    src/ir_cfg.cpp
    src/reaching_def.cpp
    src/dead_code.cpp
    src/ir_optimizer.cpp)

add_executable(demo examples/demo.cpp)
target_link_libraries(demo ircpp)

add_executable(cfg_demo examples/cfg_demo.cpp)
target_link_libraries(cfg_demo ircpp)

add_executable(test_dead_code build/test_dead_code.cpp)
target_link_libraries(test_dead_code ircpp)

add_executable(ir_optimizer build/ir_optimizer_demo.cpp)
target_link_libraries(ir_optimizer ircpp)


