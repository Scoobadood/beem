cmake_minimum_required(VERSION 3.24)
project(m6502)

set(CMAKE_CXX_STANDARD 17)

add_executable(cpu
        src/main.cpp
        include/cpu.h src/cpu.cpp
        include/addressing.h src/addressing.cpp
        include/opcodes.h src/opcodes.cpp)

