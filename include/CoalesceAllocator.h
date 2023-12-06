#ifndef COMPOSITE_MEMORY_ALLOCATOR_COALESCEALLOCATOR_H
#define COMPOSITE_MEMORY_ALLOCATOR_COALESCEALLOCATOR_H


namespace CoalesceAllocator {
    typedef unsigned int uint32;

    static constexpr uint32 PAGE_SIZE = 10 * 1024 * 1024;
    static constexpr uint32 DEADBEEF = 0xdeadbeef;
    static constexpr uint32 FEEDFACE = 0xfeedface;

#if DEBUG
    struct StatReport {
        uint32 allocCallCount;
        uint32 freeCallCount;
        uint32 totalAllocSize;
        uint32 pagesCount;
    };

    struct BlockReport {
        void* address = {};
        uint32 size = {};
        bool allocated = {};
    };
#endif

    class CoalesceAllocator {
    public:
        CoalesceAllocator();
        ~CoalesceAllocator();
        void init();
        void destroy();
        void* alloc(uint32 size);
        void free(void* p);

#if DEBUG
        [[nodiscard]] StatReport getStat() const;
        [[nodiscard]] BlockReport getNextBlock(uint32 pageNum, void* from) const;
#endif
    private:
        // BlockStart->size = sizeof(BlockStart) + size + sizeof(BlockEnd)
        struct BlockStart {
#if DEBUG
            uint32 markerStart;
#endif
            BlockStart* next;
            BlockStart* prev;
            uint32 size;
            char alloc;
#ifdef DEBUG
            uint32 markerEnd;
#endif
        };

        struct BlockEnd {
#if DEBUG
            uint32 markerStart;
#endif
            uint32 size;
#ifdef DEBUG
            uint32 markerEnd;
#endif
        };

        struct Page {
            Page* next;
            BlockStart* fh;
        };

        static BlockStart* findFreeBlock(const Page* page, uint32 size);
        static Page* createPage();
        static bool insidePage(Page* page, void* p) ;
        static void setupBlock(BlockStart *block, uint32 size, BlockStart* next, BlockStart* prev, bool free);
        static void validateBlock(BlockStart* block, bool free);

        Page* m_headPage;
#if DEBUG
        uint32 m_allocCallCount;
        uint32 m_freeCallCount;
        uint32 m_totalAllocSize;
#endif
    };
}


#endif //COMPOSITE_MEMORY_ALLOCATOR_COALESCEALLOCATOR_H
