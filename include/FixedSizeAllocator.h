#ifndef COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H
#define COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H


namespace FixedSizeAllocator {
    typedef unsigned int uint32;

    static constexpr uint32 PAGE_SIZE = 4096;

    enum FSABlockSize {
        FSA16 = 16,
        FSA32 = 32,
        FSA64 = 64,
        FSA128 = 128,
        FSA256 = 256,
        FSA512 = 512,
    };

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
        explicit FixedSizeAllocator(FSABlockSize size);
        ~FixedSizeAllocator();
        void init();
        void destroy();
        void* alloc(uint32 size);
        void free(void *p);

#ifdef DEBUG
        [[nodiscard]] StatReport getStatReport() const;
        [[nodiscard]] AllocBlocksReport getAllocBlocksReport(uint32 pageNum) const;
#endif

        const FSABlockSize BLOCK_SIZE;
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
#ifdef DEBUG
        uint32 m_allocCallCount;
        uint32 m_freeCallCount;
#endif
    };
}


#endif //COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H
