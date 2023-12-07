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

    class CompositeMemoryAllocator {
    public:
        CompositeMemoryAllocator() = default;
        ~CompositeMemoryAllocator() = default;
        void init();
        void destroy();
        void* alloc(uint32 size);
        void free(void *p);
#ifdef DEBUG
        void dumpStat() const;
        void dumpBlocks() const;
#endif
    private:
        struct VirtualAllocPage {
            VirtualAllocPage* next;
            VirtualAllocPage* prev;
        };

        FixedSizeAllocator::FixedSizeAllocator m_fixedSizeAllocators[BLOCK_TYPE_COUNT];
        CoalesceAllocator::CoalesceAllocator m_coalesceAllocator;
        VirtualAllocPage* m_virtualAllocHead{};
    };
}


#endif //COMPOSITE_MEMORY_ALLOCATOR_COMPOSITEMEMORYALLOCATOR_H
