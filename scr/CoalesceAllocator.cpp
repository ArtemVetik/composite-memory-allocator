#include "../include/CoalesceAllocator.h"

#include <windows.h>
#ifdef DEBUG
#include <iostream>
#include <cassert>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

namespace CoalesceAllocator {
    CoalesceAllocator::CoalesceAllocator() {
        m_headPage = nullptr;
#ifdef DEBUG
        m_allocCallCount = 0;
        m_freeCallCount = 0;
        m_totalAllocSize = 0;
#endif
    }

    CoalesceAllocator::~CoalesceAllocator() {
        if (m_headPage != nullptr)
            destroy();
    }

    void CoalesceAllocator::init() {
        if (m_headPage != nullptr)
            return;

        m_headPage = createPage();
    }

    void CoalesceAllocator::destroy() {
        ASSERT(m_headPage != nullptr);

        while (m_headPage) {
            Page* next = m_headPage->next;

#ifdef DEBUG
            ASSERT(m_headPage->fh->size == sizeof(BlockStart) + PAGE_SIZE + sizeof(BlockEnd));
            ASSERT(m_headPage->fh->next == nullptr);
#endif

            if (!VirtualFree(m_headPage, 0, MEM_RELEASE)) {
#ifdef DEBUG
                printf("VirtualFree failed.\n");
#endif
                return;
            }

            m_headPage = next;
        }
    }

    void* CoalesceAllocator::alloc(uint32 size) {
        ASSERT(m_headPage != nullptr);

        if (size > PAGE_SIZE)
            return nullptr;

        Page* page = m_headPage;

#ifdef DEBUG
        m_allocCallCount++;
        m_totalAllocSize += size;
#endif

        while (true) {
            //<--------(fb->size)-------->
            // ↓(fb)
            //[BlockStart][size][BlockEnd]
            BlockStart* fb = findFreeBlock(page, sizeof(BlockStart) + size + sizeof(BlockEnd));

            if (fb != nullptr) {
                validateBlock(fb, true);
                // <----------------------(fb->size)-------------------------->
                //   ↓(fb)                         ↓(nfb)
                // <[BlockStart][size][BlockEnd]> <[BlockStart][...][BlockEnd]>
                if (fb->size >= size + 2 * sizeof(BlockStart) + 2 * sizeof(BlockEnd)) {
                    auto* nfb = (BlockStart*)((BYTE*)fb + sizeof(BlockStart) + size + sizeof(BlockEnd));
                    setupBlock(nfb, fb->size - size - sizeof(BlockStart) - sizeof(BlockEnd), fb->next, fb->prev, true);
                    fb->size -= nfb->size;
                    if (page->fh == fb) page->fh = nfb;
                }
                else {
                    if (fb->next) fb->next->prev = fb->prev;
                    if (fb->prev) fb->prev->next = fb->next;
                    else page->fh = fb->next;
                }

                setupBlock(fb, fb->size, nullptr, nullptr, false);
                return (BYTE*)fb + sizeof(BlockStart);
            }

            if (page->next == nullptr)
                break;

            page = page->next;
        }

        page->next = createPage();
        page = page->next;

        uint32 blockSize = page->fh->size;
        if (page->fh->size >= size + 2 * sizeof(BlockStart) + 2 * sizeof(BlockEnd)) {
            auto* nfb = (BlockStart*)((BYTE*)page->fh + sizeof(BlockStart) + size + sizeof(BlockEnd));
            setupBlock(nfb, page->fh->size - sizeof(BlockStart) - size - sizeof(BlockEnd), nullptr, nullptr, true);
            blockSize -= nfb->size;
            page->fh = nfb;
        }
        else {
            page->fh = nullptr;
        }

        setupBlock(((BlockStart*)((BYTE*)page + sizeof(Page))), blockSize, nullptr, nullptr, false);
        return (BYTE*)page + sizeof(Page) + sizeof(BlockStart);
    }

    //             ↓(p)
    // [BlockStart][......][BlockEnd]
    void CoalesceAllocator::free(void *p) {
        ASSERT(m_headPage != nullptr);

        Page* page = m_headPage;
        while (page != nullptr) {
            // ↓(page)
            //[Page][BlockStart][..(p)..][BlockEnd]
            if (insidePage(page, p)) {
                validateBlock((BlockStart*)((BYTE*)p - sizeof(BlockStart)), false);
                break;
            }
            page = page->next;
        }

        if (page == nullptr)
            return;

        auto* cb = (BlockStart*)((BYTE*)p - sizeof(BlockStart));
        size_t lbs = ((BlockEnd*)((BYTE*)cb - sizeof(BlockEnd)))->size;
        auto* lb = (BlockStart*)((BYTE*)cb - lbs);
        auto* rb = (BlockStart*)((BYTE*)cb + cb->size);

        if (!insidePage(page, (BYTE*)lb + sizeof(BlockStart)) || lb->alloc)
            lb = nullptr;
        if (!insidePage(page, (BYTE*)rb + sizeof(BlockStart)) || rb->alloc)
            rb = nullptr;

        if (lb) validateBlock(lb, true);
        if (rb) validateBlock(rb, true);

        if (lb != nullptr) {
            lb->size += cb->size;
            cb = lb;
        }

        if (rb != nullptr) {
            cb->size += rb->size;

            if (rb->next) rb->next->prev = rb->prev;
            if (rb->prev) rb->prev->next = rb->next;
            else page->fh = rb->next;
        }

        // if no joins or only right join, update fh
        if ((lb == nullptr && rb == nullptr) || (lb == nullptr)) {
            if (page->fh)
                page->fh->prev = cb;

            cb->next = page->fh;
            cb->prev = nullptr;
            page->fh = cb;
        }

        setupBlock(cb, cb->size, cb->next, cb->prev, true);
#ifdef DEBUG
        m_freeCallCount++;
#endif
    }

    CoalesceAllocator::Page *CoalesceAllocator::createPage() {
        Page* page = (Page*)VirtualAlloc(nullptr, sizeof(Page) + sizeof(BlockStart) + PAGE_SIZE + sizeof(BlockEnd),
                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (page == nullptr) {
#ifdef DEBUG
            printf("VirtualAlloc failed.\n");
#endif
            return nullptr;
        }

        page->next = nullptr;
        page->fh = (BlockStart*)((BYTE*)page + sizeof(Page));
        setupBlock(page->fh, sizeof(BlockStart) + PAGE_SIZE + sizeof(BlockEnd), nullptr, nullptr, true);

        return page;
    }

    CoalesceAllocator::BlockStart *CoalesceAllocator::findFreeBlock(const CoalesceAllocator::Page *page, uint32 size) {
        BlockStart* block = page->fh;

        while (block != nullptr) {
            if (block->size >= size)
                return block;

            block = block->next;
        }

        return nullptr;
    }

    void CoalesceAllocator::setupBlock(BlockStart *block, uint32 size, BlockStart* next, BlockStart* prev, bool free) {
        block->alloc = free ? 0 : 1;
        block->size = size;
        block->next = next;
        block->prev = prev;
        if (block->next) block->next->prev = block;
        if (block->prev) block->prev->next = block;

        auto* end = (BlockEnd*)((BYTE*)block + size - sizeof(BlockEnd));
        end->size = size;
#ifdef DEBUG
        block->markerStart = FEEDFACE;
        block->markerEnd = FEEDFACE;
        end->markerStart = FEEDFACE;
        end->markerEnd = FEEDFACE;

        if (free) {
            *((uint32*)((BYTE*)block + sizeof(BlockStart))) = DEADBEEF;
        }
#endif
    }

    bool CoalesceAllocator::containsAddress(void *p) const {
        Page* page = m_headPage;
        while(page) {
            if (insidePage(page, p))
                return true;

            page = page->next;
        }

        return false;
    }

    bool CoalesceAllocator::insidePage(Page* page, void *p) {
        return ((BYTE *)p >= (BYTE*)page + sizeof(Page) + sizeof(BlockStart) &&
                (BYTE*) p <= (BYTE*)page + sizeof(Page) + PAGE_SIZE + sizeof(BlockStart));
    }

    void CoalesceAllocator::validateBlock(BlockStart* block, bool free) {
        ASSERT(block->markerStart == FEEDFACE);
        ASSERT(block->markerEnd == FEEDFACE);
        auto* blockEnd = (BlockEnd*)((BYTE*)block + block->size - sizeof(BlockEnd));
        ASSERT(blockEnd->markerEnd == FEEDFACE);
        ASSERT(blockEnd->markerEnd == FEEDFACE);

        if (free) {
            ASSERT(block->alloc == 0);
            ASSERT(*((uint32*)((BYTE*)block + sizeof(BlockStart))) == DEADBEEF);
        }
        else {
            ASSERT(block->alloc);
        }
    }

#ifdef DEBUG
    StatReport CoalesceAllocator::getStat() const {
        uint32 pagesCount = 0;
        Page* page = m_headPage;
        while (page) pagesCount++, page = page->next;

        return StatReport {
            m_allocCallCount,
            m_freeCallCount,
            m_totalAllocSize,
            pagesCount,
        };
    }

    BlockReport CoalesceAllocator::getNextBlock(uint32 pageNum, void* from) const {
        ASSERT(m_headPage != nullptr);

        Page* page = m_headPage;
        for (int i = 0; i < pageNum && page; ++i, page = page->next);

        if (page == nullptr)
            return BlockReport{};

        if (!from) {
            auto* block = (BlockStart*)((BYTE*)page + sizeof(Page));
            return BlockReport {
                    (BYTE*)block + sizeof(BlockStart),
                    block->size - (uint32)sizeof(BlockStart) - (uint32)sizeof(BlockEnd),
                    block->alloc != 0,
            };
        }

        ASSERT(insidePage(page, from));

        auto* fromBlock = (BlockStart*)((BYTE*)from - sizeof(BlockStart));

        auto* next = (BlockStart*)((BYTE*)fromBlock + fromBlock->size);

        if (!insidePage(page, next))
            return BlockReport{};

        return BlockReport {
                (BYTE*)next + sizeof(BlockStart),
            next->size - (uint32)sizeof(BlockStart) - (uint32)sizeof(BlockEnd),
            next->alloc != 0,
        };
    }

#endif // DEBUG
}
