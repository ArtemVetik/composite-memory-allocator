#pragma once
#include <chrono>
#include <vector>
#include <unordered_map>
#include <map>

using Clock = std::chrono::steady_clock;

struct XorShift32
{
    uint32_t state = 0x12345678;

    uint32_t next()
    {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    uint32_t range(uint32_t max)
    {
        return next() % max;
    }
};

struct BenchmarkConfig
{
    int iterations = 5'000'000;
    int maxLiveAllocs = 10000;
    float allocChance = 0.6f;
    int minSize = 524;
    int maxSize = 8096;
};

template<typename TAllocator>
double benchmark_random(TAllocator& alloc, const BenchmarkConfig& cfg)
{
    std::vector<std::byte*> live;
    live.reserve(cfg.maxLiveAllocs);

    XorShift32 rng;

    auto t0 = Clock::now();

    for (int i = 0; i < cfg.iterations; ++i)
    {
        bool doAlloc =
            live.empty() ||
            (live.size() < cfg.maxLiveAllocs &&
                rng.next() < cfg.allocChance * UINT32_MAX);

        if (doAlloc)
        {
            int size = cfg.minSize + rng.range(cfg.maxSize - cfg.minSize + 1);
            std::byte* p = alloc.allocate(size);
            live.push_back(p);
        }
        else
        {
            int idx = rng.range((uint32_t)live.size());
            alloc.deallocate(live[idx], 1);
            live[idx] = live.back();
            live.pop_back();
        }
    }

    for (std::byte* p : live)
        alloc.deallocate(p, 1);

    auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

template<typename T, typename Alloc>
double benchmark_vector(const BenchmarkConfig& cfg)
{
    std::vector<T, Alloc> v;
    XorShift32 rng;

    auto t0 = std::chrono::steady_clock::now();

    for (uint32_t i = 0; i < cfg.iterations; ++i)
    {
        bool doPush =
            v.empty() ||
            (v.size() < cfg.maxLiveAllocs &&
                rng.next() < uint32_t(cfg.allocChance * UINT32_MAX));

        if (doPush)
        {
            T value = rng.next();
            v.push_back(value);
        }
        else
        {
            v.pop_back();
        }
    }

    while (!v.empty())
        v.pop_back();

    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

template<typename Alloc>
double benchmark_unordered_map(const BenchmarkConfig& cfg)
{
    using Map = std::unordered_map<
        uint32_t,
        uint32_t,
        std::hash<uint32_t>,
        std::equal_to<uint32_t>,
        Alloc>;

    Map map;
    map.reserve(cfg.maxLiveAllocs / 2);

    XorShift32 rng;

    auto t0 = Clock::now();

    for (uint32_t i = 0; i < cfg.iterations; ++i)
    {
        bool doInsert =
            map.empty() ||
            (map.size() < cfg.maxLiveAllocs &&
                rng.next() < uint32_t(cfg.allocChance * UINT32_MAX));

        if (doInsert)
        {
            uint32_t k = rng.next();
            map.emplace(k, k);
        }
        else
        {
            auto it = map.begin();
            std::advance(it, rng.range((uint32_t)map.size()));
            map.erase(it);
        }
    }

    auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

template<typename Alloc>
double benchmark_map(const BenchmarkConfig& cfg)
{
    using Map = std::map<uint32_t, uint32_t, std::less<uint32_t>, Alloc>;

    Map map;
    XorShift32 rng;

    auto t0 = Clock::now();

    for (uint32_t i = 0; i < cfg.iterations; ++i)
    {
        bool doInsert =
            map.empty() ||
            (map.size() < cfg.maxLiveAllocs &&
                rng.next() < uint32_t(cfg.allocChance * UINT32_MAX));

        if (doInsert)
        {
            uint32_t k = rng.next();
            map.emplace(k, k);
        }
        else
        {
            auto it = map.begin();
            std::advance(it, rng.range((uint32_t)map.size()));
            map.erase(it);
        }
    }

    auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}