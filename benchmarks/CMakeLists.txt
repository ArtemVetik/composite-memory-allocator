add_executable(benchmarks src/main.cpp)

target_include_directories(benchmarks PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(benchmarks PUBLIC composite_memory_allocator)