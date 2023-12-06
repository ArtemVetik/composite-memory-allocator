#include "../include/FixedSizeAllocator.h"

#include <windows.h>
#include <iostream>
#ifdef DEBUG
#include <cassert>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

namespace FixedSizeAllocator {
    FixedSizeAllocator::FixedSizeAllocator(FSABlockSize size) : BLOCK_SIZE(size) {
        m_headPage = nullptr;

#if DEBUG
        m_allocCallCount = 0;
        m_freeCallCount = 0;
#endif
    }

    FixedSizeAllocator::~FixedSizeAllocator() {
        if (m_headPage != nullptr)
            destroy();
    }

    void FixedSizeAllocator::init() {
        if (m_headPage != nullptr)
            return;

        m_headPage = createPage();
    }

    void FixedSizeAllocator::destroy() {
        ASSERT(m_headPage != nullptr);

        uint32 pageNum = 0;
        while (m_headPage) {
            Page* next = m_headPage->next;

#ifdef DEBUG
            AllocBlocksReport report = getAllocBlocksReport(pageNum++);
            ASSERT(report.count == 0);
#endif

            if (!VirtualFree(m_headPage, 0, MEM_RELEASE)) {
                printf("VirtualFree failed.\n");
                return;
            }

            m_headPage = next;
        }
    }

    void* FixedSizeAllocator::alloc(uint32 size) {
        ASSERT(m_headPage != nullptr);

        if (size > BLOCK_SIZE)
            return nullptr;

#if DEBUG
        m_allocCallCount++;
#endif

        Page* page = m_headPage;
        while (true) {
            if (page->numInit < PAGE_SIZE) {
                page->numInit++;
                return (BYTE*)page + sizeof(Page) + (page->numInit - 1) * BLOCK_SIZE;
            }

            if (page->fh >= 0) {
                int fh = page->fh;
                page->fh = ((Block*)((BYTE*)page + page->fh * BLOCK_SIZE))->freeIndex;
                return (BYTE*)page + sizeof(Page) + fh * BLOCK_SIZE;
            }

            if (page->next == nullptr)
                break;

            page = page->next;
        }

        Page* newPage = createPage();

        if (newPage == nullptr)
            return nullptr;

        newPage->numInit = 1;
        page->next = newPage;

        return (BYTE*)newPage + sizeof(Page);
    }

    void FixedSizeAllocator::free(void *p) {
        ASSERT(m_headPage != nullptr);
#if DEBUG
        m_freeCallCount++;
#endif

        Page* page = m_headPage;
        while (true) {
            int blockNum = (int)((BYTE*)p - (BYTE*)page - sizeof(Page)) / BLOCK_SIZE;

            if (blockNum >= 0 && blockNum <= PAGE_SIZE) {
#ifdef DEBUG
                int fh = page->fh;
                while (fh >= 0) {
                    ASSERT(blockNum != fh);
                    fh = ((Block*)((BYTE*)page + sizeof(Page) + fh * BLOCK_SIZE))->freeIndex;
                }
#endif
                ((Block*)((BYTE*)page + sizeof(Page) + blockNum * BLOCK_SIZE))->freeIndex = page->fh;
                page->fh = blockNum;
                return;
            }

            if (page->next == nullptr)
                break;

            page = page->next;
        }
    }

    FixedSizeAllocator::Page *FixedSizeAllocator::createPage() const {
        Page* page = (Page*)VirtualAlloc(nullptr, sizeof(Page) + BLOCK_SIZE * PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (page == nullptr) {
            printf("VirtualAlloc failed.\n");
            return nullptr;
        }

        page->next = nullptr;
        page->numInit = 0;
        page->fh = -1;

        return page;
    }

#ifdef DEBUG
    StatReport FixedSizeAllocator::getStatReport() const {
        ASSERT(m_headPage != nullptr);

        Page* page = m_headPage;
        uint32 pageCount = 0;
        uint32 freeCount = 0;

        while (page != nullptr) {
            int fh = page->fh;
            while (fh >= 0) {
                freeCount++;
                fh = ((Block*)((BYTE*)page + sizeof(Page) + fh * BLOCK_SIZE))->freeIndex;
            }
            freeCount += PAGE_SIZE - page->numInit;

            page = page->next;
            pageCount++;
        }

        return StatReport {
            m_allocCallCount,
            m_freeCallCount,
            freeCount,
            pageCount,
        };
    }

    AllocBlocksReport FixedSizeAllocator::getAllocBlocksReport(uint32 pageNum) const {
        ASSERT(m_headPage != nullptr);

        Page* page = m_headPage;
        for (int i = 0; i < pageNum && page; ++i, page = page->next);

        if (!page)
            return AllocBlocksReport {};

        bool blocks[PAGE_SIZE];

        int fh = page->fh;
        while (fh >= 0) {
            blocks[fh] = true;
            fh = ((Block*)((BYTE*)page + sizeof(Page) + fh * BLOCK_SIZE))->freeIndex;
        }

        AllocBlocksReport report{};
        for (int i = 0; i < page->numInit; ++i) {
            if (!blocks[i]) {
                report.blocks[report.count++] = (BYTE*)page + sizeof(Page) + i * BLOCK_SIZE;
            }
        }

        return report;
    }
#endif // DEBUG
}