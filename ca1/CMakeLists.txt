cmake_minimum_required(VERSION 3.29)
project(ca1)

set(CMAKE_CXX_STANDARD 98)

include_directories(src)

add_executable(cpusim
    src/CPU.cpp
    src/CPU.h
    src/cpusim.cpp)
