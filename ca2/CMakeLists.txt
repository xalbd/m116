cmake_minimum_required(VERSION 3.29)
project(ca2)

set(CMAKE_CXX_STANDARD 98)

add_custom_target(ca2
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        COMMAND make
)

add_executable(predict
        src/predict.cc
        src/trace.cc
)