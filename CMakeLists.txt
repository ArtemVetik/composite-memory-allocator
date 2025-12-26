cmake_minimum_required(VERSION 3.26)
project(composite_memory_allocator LANGUAGES CXX)

add_library(composite_memory_allocator STATIC
    src/CoalesceAllocator.cpp
    src/CompositeMemoryAllocator.cpp
    src/FixedSizeAllocator.cpp
)

target_include_directories(composite_memory_allocator
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(composite_memory_allocator PUBLIC cxx_std_17)
target_compile_definitions(composite_memory_allocator PUBLIC ALLOCATORS_DEBUG)

add_subdirectory(google-tests)
add_subdirectory(benchmarks)
add_subdirectory(example)

add_test(
    NAME composite_memory_allocator
    COMMAND Google_Tests_run
)