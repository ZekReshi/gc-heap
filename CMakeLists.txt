cmake_minimum_required(VERSION 3.28)
project(GC)

set(CMAKE_CXX_STANDARD 17)

add_executable(GC main.cpp
        Heap.cpp
        Heap.h)
