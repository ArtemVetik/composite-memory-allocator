#include "../include/FixedSizeAllocator.h"

FixedSizeAllocator::FixedSizeAllocator(FSABlockSize size) : kBlockSize(size) {
    headPage_ = nullptr;

#if DEBUG
    allocCallCount_ = 0;
    freeCallCount_ = 0;
#endif
}

FixedSizeAllocator::~FixedSizeAllocator() {
    if (headPage_ != nullptr)
        destroy();
}

void FixedSizeAllocator::init() {
    if (headPage_ != nullptr)
        return;

    headPage_ = createPage();
}

void FixedSizeAllocator::destroy() {
#ifdef DEBUG
    assert(headPage_ != nullptr);
#endif

    Page* page = headPage_;

    while (true) {
        Page* next = page->next;
#ifdef DEBUG
        assert(page->freeBlockCount == kPageSize);
#endif
        if (!VirtualFree(page, 0, MEM_RELEASE)) {
            printf("VirtualFree failed.\n");
            return;
        }

        if (next == nullptr)
            break;

        page = next;
    }

    headPage_ = nullptr;
}

void *FixedSizeAllocator::alloc(size_t size) {
#ifdef DEBUG
    assert(headPage_ != nullptr);
#endif

    if (size > kBlockSize)
        return nullptr;

#if DEBUG
    allocCallCount_++;
#endif

    Page* page = headPage_;
    while (true) {
        if (page->numInit < kPageSize) {
            page->numInit++;
#if DEBUG
            page->freeBlockCount--;
#endif
            return (BYTE*)page + sizeof(Page) + (page->numInit - 1) * kBlockSize;
        }

        if (page->fh >= 0) {
            int fh = page->fh;
            page->fh = ((Block*)(page + page->fh * kBlockSize))->freeIndex;
#if DEBUG
            page->freeBlockCount--;
#endif
            return (BYTE*)page + sizeof(Page) + fh * kBlockSize;
        }

        if (page->next == nullptr)
            break;

        page = page->next;
    }

    Page* newPage = createPage();

    if (newPage == nullptr)
        return nullptr;

    newPage->numInit = 1;
#if DEBUG
    newPage->freeBlockCount--;
#endif
    page->next = newPage;

    return (BYTE*)newPage + sizeof(Page);
}

void FixedSizeAllocator::free(void *p) {
#if DEBUG
    assert(headPage_ != nullptr);
    freeCallCount_++;
#endif

    Page* page = headPage_;
    while (true) {
        int blockNum = (int)((BYTE*)p - (BYTE*)page - sizeof(Page)) / kBlockSize;

        if (blockNum >= 0 && blockNum <= kPageSize) {
#ifdef DEBUG
        int fh = page->fh;
        while (fh >= 0) {
            assert(blockNum != fh);
            fh = ((Block*)((BYTE*)page + sizeof(Page) + fh * kBlockSize))->freeIndex;
        }
#endif
            ((Block*)((BYTE*)page + sizeof(Page) + blockNum * kBlockSize))->freeIndex = page->fh;
            page->fh = blockNum;
#if DEBUG
            page->freeBlockCount++;
#endif
            return;
        }

        if (page->next == nullptr)
            break;

        page = page->next;
    }
}

#ifdef DEBUG
void FixedSizeAllocator::dumpStat() const {
    assert(headPage_ != nullptr);

    printf("---------------dumpStat---------------\n");
    printf("FSA: %d\n", kBlockSize);

    Page* page = headPage_;
    unsigned int i = 1;
    unsigned int freeCount = 0;

    while (page != nullptr) {
        printf("Page %u: %u free blocks\n", i, page->freeBlockCount);
        freeCount += page->freeBlockCount;
        page = page->next;
        i++;
    }

    printf("Total pages: %u\n", i - 1);
    printf("Total free count: %u\n", freeCount);
    printf("Allocation calls: %u\n", allocCallCount_);
    printf("Free calls: %u\n", freeCallCount_);
    printf("----------------------------------------\n");
}

void FixedSizeAllocator::dumpBlocks() const {
    assert(headPage_ != nullptr);

    printf("---------------dumpBlocks---------------\n");
    Page* page = headPage_;
    unsigned int i = 1;
    while (page != nullptr) {
        int blocks[kPageSize];

        int fh = page->fh;
        while (fh >= 0) {
            blocks[fh] = 1;
            fh = ((Block*)((BYTE*)page + sizeof(Page) + fh * kBlockSize))->freeIndex;
        }

        for (int j = 0; j < page->numInit; ++j) {
            if (blocks[j] == 0) {
                printf("Page %u\tBlock %p\n", i, (BYTE*)page + sizeof(Page) + j * kBlockSize);
            }
        }

        page = page->next;
        i++;
    }
    printf("----------------------------------------\n");
}
#endif // DEBUG

FixedSizeAllocator::Page *FixedSizeAllocator::createPage() const {
    Page* page = (Page*)VirtualAlloc(nullptr, kBlockSize * kPageSize + sizeof(Page), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (page == nullptr) {
        printf("VirtualAlloc failed.\n");
        return nullptr;
    }

    page->next = nullptr;
    page->numInit = 0;
    page->fh = -1;

#if DEBUG
    page->freeBlockCount = kPageSize;
#endif

    return  page;
}
