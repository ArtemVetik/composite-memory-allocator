cmake_minimum_required(VERSION 3.26)
project(composite_memory_allocator)

set(CMAKE_CXX_STANDARD 17)

add_executable(composite_memory_allocator scr/main.cpp
        include/FixedSizeAllocator.h
        scr/FixedSizeAllocator.cpp)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif()