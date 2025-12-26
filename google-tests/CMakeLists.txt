add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(Google_Tests_run
            FixedSizeAllocatorTests.cpp
            CoalesceAllocatorTests.cpp
            CompositeMemoryAllocatorTests.cpp
    )

    target_link_libraries(Google_Tests_run composite_memory_allocator gtest gtest_main)
endif()