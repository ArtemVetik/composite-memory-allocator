#include "../include/FixedSizeAllocator.h"

#include <windows.h>
#ifdef DEBUG
#include <iostream>
#include <cassert>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

namespace FixedSizeAllocator {
    FixedSizeAllocator::FixedSizeAllocator() {
        m_blockSize = -1;
        m_headPage = nullptr;
#ifdef DEBUG
        m_allocCallCount = 0;
        m_freeCallCount = 0;
#endif
    }

    FixedSizeAllocator::~FixedSizeAllocator() {
        if (m_headPage != nullptr)
            destroy();
    }

    void FixedSizeAllocator::init(uint32 blockSize) {
        if (m_headPage != nullptr) {
            return;
        }

        m_blockSize = blockSize;
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
#ifdef DEBUG
                printf("VirtualFree failed.\n");
#endif
                return;
            }

            m_headPage = next;
        }
    }

    void* FixedSizeAllocator::alloc(uint32 size) {
        ASSERT(m_headPage != nullptr);

        if (size > m_blockSize)
            return nullptr;

#ifdef DEBUG
        m_allocCallCount++;
#endif

        Page* page = m_headPage;
        while (true) {
            if (page->numInit < PAGE_SIZE) {
                page->numInit++;
                return (BYTE*)page + sizeof(Page) + (page->numInit - 1) * m_blockSize;
            }

            if (page->fh >= 0) {
                int fh = page->fh;
                page->fh = ((Block*)((BYTE*)page + sizeof(Page) + page->fh * m_blockSize))->freeIndex;
                return (BYTE*)page + sizeof(Page) + fh * m_blockSize;
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
#ifdef DEBUG
        m_freeCallCount++;
#endif

        Page* page = m_headPage;
        while (true) {
            int blockNum = (int)((BYTE*)p - (BYTE*)page - sizeof(Page)) / (int)m_blockSize;

            if (blockNum >= 0 && blockNum <= PAGE_SIZE) {
#ifdef DEBUG
                int fh = page->fh;
                while (fh >= 0) {
                    ASSERT(blockNum != fh);
                    fh = ((Block*)((BYTE*)page + sizeof(Page) + fh * m_blockSize))->freeIndex;
                }
#endif
                ((Block*)((BYTE*)page + sizeof(Page) + blockNum * m_blockSize))->freeIndex = page->fh;
                page->fh = blockNum;
                return;
            }

            if (page->next == nullptr)
                break;

            page = page->next;
        }
    }

    uint32 FixedSizeAllocator::getBlockSize() const {
        return m_blockSize;
    }

    bool FixedSizeAllocator::containsAddress(void *p) const {
        Page* page = m_headPage;
        while(page) {
            if (p >= (BYTE*)page + sizeof(Page) && p < (BYTE*)page + sizeof(Page) + m_blockSize * PAGE_SIZE)
                return true;

            page = page->next;
        }

        return false;
    }

    FixedSizeAllocator::Page *FixedSizeAllocator::createPage() const {
        Page* page = (Page*)VirtualAlloc(nullptr, sizeof(Page) + m_blockSize * PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (page == nullptr) {
#ifdef DEBUG
            printf("VirtualAlloc failed.\n");
#endif
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
                fh = ((Block*)((BYTE*)page + sizeof(Page) + fh * m_blockSize))->freeIndex;
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
            fh = ((Block*)((BYTE*)page + sizeof(Page) + fh * m_blockSize))->freeIndex;
        }

        AllocBlocksReport report{};
        for (int i = 0; i < page->numInit; ++i) {
            if (!blocks[i]) {
                report.blocks[report.count++] = (BYTE*)page + sizeof(Page) + i * m_blockSize;
            }
        }

        return report;
    }

#endif // DEBUG
}