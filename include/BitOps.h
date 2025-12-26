#pragma once
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace BitOps {

    __forceinline uint32_t msb_index(uint32_t v)
    {
        if (!v) return v;

#if defined(_MSC_VER)
        DWORD idx;
        _BitScanReverse(&idx, v);
        return (uint32_t)idx;
#elif defined(__GNUC__) || defined(__clang__)
        return 31u - __builtin_clz(v);
#else
        uint32_t idx = 0;
        while (v >>= 1) ++idx;
        return idx;
#endif
    }

    __forceinline uint32_t log2_floor(uint32_t v)
    {
        return msb_index(v);
    }

    __forceinline uint32_t log2_ceil(uint32_t v)
    {
        return v <= 1 ? 0 : msb_index(v - 1) + 1;
    }

}