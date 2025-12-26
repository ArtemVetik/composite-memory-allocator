#include <crtdbg.h>
#include <sstream>
#include <iostream>

#include <MemoryAllocatorT.h>
#include "BenchmarkFuncs.h"

template <typename T>
struct StdAllocator {
    using value_type = T;

    StdAllocator() = default;
    ~StdAllocator() = default;

    template <typename U>
    StdAllocator(const StdAllocator<U>& other) noexcept
    {
    }

    template <typename U>
    StdAllocator(StdAllocator<U>&& other)
    {
    }

    template <typename U>
    StdAllocator& operator = (const StdAllocator<U>& rhs) = delete;
    template <typename U>
    StdAllocator& operator = (StdAllocator<U>&& lhs) = delete;

    T* allocate(std::size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t) {
        ::operator delete(p);
    }

    template <typename U>
    struct rebind {
        using other = StdAllocator<U>;
    };
};

void runRawTest(const char* name, const BenchmarkConfig& cfg, StdAllocator<std::byte>& stdAllocator, MemoryAllocator::MemoryAllocatorT<std::byte>& customAllocator)
{
    printf("======== %s ========\n", name);
    printf("StdAllocator:    %lf ms\n", benchmark_random(stdAllocator, cfg));
    printf("CustomAllocator: %lf ms\n", benchmark_random(customAllocator, cfg));
    printf("============================\n\n");
}

template<typename T>
void runVectorTest(const char* name, const BenchmarkConfig& cfg)
{
    printf("======== %s ========\n", name);
    printf("StdAllocator:    %lf ms\n", benchmark_vector<T, StdAllocator<T>>(cfg));
    printf("CustomAllocator: %lf ms\n", benchmark_vector<T, MemoryAllocator::MemoryAllocatorT<T>>(cfg));
    printf("============================\n\n");
}

void runUnorderedMapTest(const char* name, const BenchmarkConfig& cfg)
{
    printf("======== %s ========\n", name);
    printf("StdAllocator:    %lf ms\n", benchmark_unordered_map<StdAllocator<std::pair<const uint32_t, uint32_t>>>(cfg));
    printf("CustomAllocator: %lf ms\n", benchmark_unordered_map<MemoryAllocator::MemoryAllocatorT<std::pair<const uint32_t, uint32_t>>>(cfg));
    printf("============================\n\n");
}

void runMapTest(const char* name, const BenchmarkConfig& cfg)
{
    printf("======== %s ========\n", name);
    printf("StdAllocator:    %lf ms\n", benchmark_map<StdAllocator<std::pair<const uint32_t, uint32_t>>>(cfg));
    printf("CustomAllocator: %lf ms\n", benchmark_map<MemoryAllocator::MemoryAllocatorT<std::pair<const uint32_t, uint32_t>>>(cfg));
    printf("============================\n\n");
}

int main()
{
    StdAllocator<std::byte> stdAllocator;
    MemoryAllocator::MemoryAllocatorT<std::byte> customAllocator;

    for (uint32 i = 0; i < 1'000'000; i++)
    {
        size_t size = 4 + (i * 8u) % (1024 * 1024);

        std::byte* p0 = stdAllocator.allocate(size);
        stdAllocator.deallocate(p0, size);

        std::byte* p1 = customAllocator.allocate(size);
        customAllocator.deallocate(p1, size);
    }

    {
        BenchmarkConfig cfg = {};
        cfg.iterations = 10'000'000;
        cfg.maxLiveAllocs = 100'000;
        cfg.allocChance = 0.6f;
        cfg.minSize = 1;
        cfg.maxSize = 512;

        runRawTest("SmallAlloc", cfg, stdAllocator, customAllocator);
    }

    {
        BenchmarkConfig cfg = {};
        cfg.iterations = 1'000'000;
        cfg.maxLiveAllocs = 10000;
        cfg.allocChance = 0.6f;
        cfg.minSize = 514;
        cfg.maxSize = 1024 * 1024;

        runRawTest("MediuamAlloc", cfg, stdAllocator, customAllocator);
    }

    {
        BenchmarkConfig cfg = {};
        cfg.iterations = 100'000;
        cfg.maxLiveAllocs = 1000;
        cfg.allocChance = 0.6f;
        cfg.minSize = 1024 * 1024;
        cfg.maxSize = 64 * 1024 * 1024;
        
        runRawTest("BigAlloc", cfg, stdAllocator, customAllocator);
    }

    {
        BenchmarkConfig cfg = {};
        cfg.iterations = 1'000'000;
        cfg.maxLiveAllocs = 1000;
        cfg.allocChance = 0.6f;
        cfg.minSize = 16;
        cfg.maxSize = 32 * 1024 * 1024;

        runRawTest("MixedAlloc", cfg, stdAllocator, customAllocator);
    }

    {
        BenchmarkConfig cfg = {};
        cfg.iterations = 10'000'000;
        cfg.maxLiveAllocs = 10'000;
        cfg.allocChance = 0.4f;

        runVectorTest<int>("LowVector", cfg);
        runUnorderedMapTest("LowUnorderedMap", cfg);
        runMapTest("LowMap", cfg);
    }

    {
        BenchmarkConfig cfg = {};
        cfg.iterations = 1'000'000;
        cfg.maxLiveAllocs = 1'000;
        cfg.allocChance = 0.6f;
        
        runVectorTest<int>("MediumVector", cfg);
        runUnorderedMapTest("MediumUnorderedMap", cfg);
        runMapTest("MediumMap", cfg);
    }

    {
        BenchmarkConfig cfg = {};
        cfg.iterations = 1'000'000;
        cfg.maxLiveAllocs = 1'000;
        cfg.allocChance = 0.9f;

        runVectorTest<int>("LargeVector", cfg);
        runUnorderedMapTest("LargeUnorderedMap", cfg);
        runMapTest("LargeMap", cfg);
    }

	return 0;
}