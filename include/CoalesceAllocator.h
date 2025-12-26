#ifndef COMPOSITE_MEMORY_ALLOCATOR_COALESCEALLOCATOR_H
#define COMPOSITE_MEMORY_ALLOCATOR_COALESCEALLOCATOR_H

#include "Types.h"

namespace CoalesceAllocator {
    static constexpr uint32 NUM_BINS = 24;
    static constexpr uint32 PAGE_SIZE = 1u << NUM_BINS; // 16 * 1024 * 1024
    static constexpr uint32 DEADBEEF = 0xdeadbeef;
    static constexpr uint32 FEEDFACE = 0xfeedface;

#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
    struct StatReport {
        uint64 allocCallCount = 0;
        uint64 freeCallCount = 0;
        uint64 totalAllocSize = 0;
        uint32 pagesCount = 0;
    };

    struct BlockReport {
        void* address = nullptr;
        uint32 size = 0;
        bool allocated = false;
    };
#endif

    class CoalesceAllocator {
    public:
        CoalesceAllocator();
        ~CoalesceAllocator();

        CoalesceAllocator(const CoalesceAllocator&) = delete;
        CoalesceAllocator& operator = (const CoalesceAllocator&) = delete;
        CoalesceAllocator(CoalesceAllocator&&) = delete;
        CoalesceAllocator& operator = (CoalesceAllocator&&) = delete;

        void init();
        void destroy();
        void* alloc(uint32 size);
        void free(void* p);
        bool containsAddress(void* p) const;
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
        [[nodiscard]] StatReport getStat() const { return m_StatReport; }
        [[nodiscard]] BlockReport getNextBlock(uint32 pageNum, void* from) const;
        [[nodiscard]] static uint32 getBlockStartSize() { return sizeof(BlockStart); }
        [[nodiscard]] static uint32 getBlockEndSize() { return sizeof(BlockEnd); }
#endif
    private:
        // BlockStart->size = sizeof(BlockStart) + size + sizeof(BlockEnd)
        struct BlockStart {
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
            uint32 markerStart;
#endif
            BlockStart* next;
            BlockStart* prev;
            uint32 size;
            char alloc;
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
            uint32 markerEnd;
#endif
        };

        struct BlockEnd {
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
            uint32 markerStart;
#endif
            uint32 size;
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
            uint32 markerEnd;
#endif
        };

        struct Page {
            Page* next;
            BlockStart* fh[NUM_BINS];
        };

        static uint32 binIndex(uint32 size);
        static BlockStart* findFreeBlock(const Page* page, uint32 size, uint32& outBinIdx);
        static Page* createPage(uint32& outBinIdx);
        static bool insidePage(Page* page, void* p) ;
        static void setupBlock(BlockStart *block, uint32 size, BlockStart* next, BlockStart* prev, bool free);

        Page* m_headPage;
#if !defined(NDEBUG) && defined(ALLOCATORS_DEBUG)
        StatReport m_StatReport;
#endif
    };
}


#endif //COMPOSITE_MEMORY_ALLOCATOR_COALESCEALLOCATOR_H
