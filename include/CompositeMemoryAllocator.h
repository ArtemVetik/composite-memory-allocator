#ifndef COMPOSITE_MEMORY_ALLOCATOR_COMPOSITEMEMORYALLOCATOR_H
#define COMPOSITE_MEMORY_ALLOCATOR_COMPOSITEMEMORYALLOCATOR_H

#include "FixedSizeAllocator.h"
#include "CoalesceAllocator.h"

namespace CompositeMemoryAllocator {
    typedef unsigned int uint32;

    static constexpr uint32 BLOCK_TYPE_COUNT = 6;

    enum FSABlockSize {
        FSA16 = 4,
        FSA32 = 5,
        FSA64 = 6,
        FSA128 = 7,
        FSA256 = 8,
        FSA512 = 9,
    };

    class CompositeMemoryAllocatorTests {
    public:
        CompositeMemoryAllocatorTests() = default;
        ~CompositeMemoryAllocatorTests() = default;
        void init();
        void destroy();
        void* alloc(uint32 size);
        void free(void *p);

    private:
        FixedSizeAllocator::FixedSizeAllocator m_fixedSizeAllocators[BLOCK_TYPE_COUNT];
        CoalesceAllocator::CoalesceAllocator m_coalesceAllocator;
    };
}


#endif //COMPOSITE_MEMORY_ALLOCATOR_COMPOSITEMEMORYALLOCATOR_H
