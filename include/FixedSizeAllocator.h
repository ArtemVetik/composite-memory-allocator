#ifndef COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H
#define COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H


namespace FixedSizeAllocator {
    typedef unsigned int uint32;

    static constexpr uint32 PAGE_SIZE = 4096;

#ifdef DEBUG
    struct StatReport {
        uint32 allocCallCount;
        uint32 freeCallCount;
        uint32 freeBlockCount;
        uint32 pagesCount;
    };

    struct AllocBlocksReport {
        void *blocks[PAGE_SIZE];
        uint32 count;
    };
#endif

    class FixedSizeAllocator {
    public:
        FixedSizeAllocator();
        ~FixedSizeAllocator();
        void init(uint32 blockSize);
        void destroy();
        void* alloc(uint32 size);
        void free(void *p);
        [[nodiscard]] uint32 getBlockSize() const;
        bool containsAddress(void* p) const;
#ifdef DEBUG
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
#ifdef DEBUG
        uint32 m_allocCallCount;
        uint32 m_freeCallCount;
#endif
    };
}


#endif //COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H
