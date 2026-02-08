cmake_minimum_required(VERSION 3.10)
project(A3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(A3 main.cpp)

target_compile_options(A3 PRIVATE -O3)