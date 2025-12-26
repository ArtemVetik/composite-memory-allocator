#ifndef COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H
#define COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H

#include "Types.h"

namespace FixedSizeAllocator {

    static constexpr uint32 PAGE_SIZE = 4096u;

#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
    struct StatReport {
        uint64 allocCallCount = 0;
        uint64 freeCallCount = 0;
        uint32 freeBlockCount = 0;
        uint32 pagesCount = 0;
    };

    struct AllocBlocksReport {
        void *blocks[PAGE_SIZE];
        uint32 count = 0;
    };
#endif

    class FixedSizeAllocator {
    public:
        FixedSizeAllocator();
        ~FixedSizeAllocator();

        FixedSizeAllocator(const FixedSizeAllocator&) = delete;
        FixedSizeAllocator& operator = (const FixedSizeAllocator&) = delete;
        FixedSizeAllocator(FixedSizeAllocator&&) = delete;
        FixedSizeAllocator& operator = (FixedSizeAllocator&&) = delete;

        void init(uint32 blockSize);
        void destroy();
        void* alloc(uint32 size);
        void free(void *p);
        [[nodiscard]] uint32 getBlockSize() const;
        bool containsAddress(void* p) const;
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
        [[nodiscard]] StatReport getStatReport() const;
        [[nodiscard]] AllocBlocksReport getAllocBlocksReport(uint32 pageNum) const;
#endif

    private:
        struct Page {
            Page *next;
            int numInit;
            int fh;
        };
        struct Block {
            int freeIndex;
        };

        [[nodiscard]] Page *createPage() const;

        Page *m_headPage;
        uint32 m_blockSize;
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
        StatReport m_StatReport;
#endif
    };
}


#endif //COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H
