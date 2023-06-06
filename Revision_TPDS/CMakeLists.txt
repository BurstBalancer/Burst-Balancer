cmake_minimum_required(VERSION 3.14)
project(CPU)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2 -g -rdynamic -mavx2")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

include_directories(.)
include_directories(Common)
include_directories(algos)
include_directories(Bench)

add_executable(delay delay.cpp)
add_executable(duration duration.cpp)
add_executable(loss loss.cpp)
add_executable(percent percent.cpp)
add_executable(active active.cpp)
add_executable(thld thld.cpp)
