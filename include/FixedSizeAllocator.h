#ifndef COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H
#define COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H


#include <windows.h>
#include <iostream>
#include <cassert>

constexpr int kPageSize = 4096;

enum FSABlockSize {
    kFSA16 = 16,
    kFSA32 = 32,
    kFSA64 = 64,
    kFSA128 = 128,
    kFSA256 = 256,
    kFSA512 = 512,
};

class FixedSizeAllocator {
public:
    FixedSizeAllocator(FSABlockSize size);
    ~FixedSizeAllocator();
    void init();
    void destroy();
    void* alloc(size_t size);
    void free(void* p);

#if DEBUG
    void dumpStat() const;
    void dumpBlocks() const;
#endif

    const FSABlockSize kBlockSize;
private:
    struct Page {
        Page* next;
        int numInit;
        int fh;
#if DEBUG
        unsigned int freeBlockCount;
#endif
    };

    struct Block {
        int freeIndex;
    };

    Page* createPage() const;

    Page* headPage_;

#if DEBUG
    unsigned int allocCallCount_;
    unsigned int freeCallCount_;
#endif
};


#endif //COMPOSITE_MEMORY_ALLOCATOR_FIXEDSIZEALLOCATOR_H
