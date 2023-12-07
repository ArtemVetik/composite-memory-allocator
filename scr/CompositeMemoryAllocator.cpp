#include "../include/CompositeMemoryAllocator.h"

#include <windows.h>

#ifdef DEBUG
#include <iostream>
#include <cassert>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

namespace CompositeMemoryAllocator {

    void CompositeMemoryAllocator::CompositeMemoryAllocator::init() {
        for (int i = 0; i < BLOCK_TYPE_COUNT; ++i) {
            m_fixedSizeAllocators[i].init(1 << (FSABlockSize::FSA16 + i));
        }

        m_coalesceAllocator.init();
    }

    void CompositeMemoryAllocator::CompositeMemoryAllocator::destroy() {
        for (auto & m_fixedSizeAllocator : m_fixedSizeAllocators)
            m_fixedSizeAllocator.destroy();

        m_coalesceAllocator.destroy();

        ASSERT(m_virtualAllocHead == nullptr);
    }

    void* CompositeMemoryAllocator::CompositeMemoryAllocator::alloc(uint32 size) {
        if (size > 0 && size <= 1 << FSABlockSize::FSA512) {
//            int index = __builtin_clz((size - 1) >> 3) ^ 31;
//            return m_fixedSizeAllocators[index].alloc(size);

            for (auto &fsa : m_fixedSizeAllocators)
                if (fsa.getBlockSize() >= size)
                    return fsa.alloc(size);
        }
        else if (size <= CoalesceAllocator::PAGE_SIZE) {
            return m_coalesceAllocator.alloc(size);
        }
        else {
            auto* page = (VirtualAllocPage*)VirtualAlloc(nullptr, size + sizeof(VirtualAllocPage), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            page->next = m_virtualAllocHead;
            if (page->next) page->next->prev = page;
            page->prev = nullptr;
            m_virtualAllocHead = page;
            return (BYTE*)page + sizeof(VirtualAllocPage);
        }
    }

    void CompositeMemoryAllocator::CompositeMemoryAllocator::free(void *p) {
        for (auto &fsa : m_fixedSizeAllocators) {
            if (fsa.containsAddress(p)) {
                fsa.free(p);
                return;
            }
        }

        if (m_coalesceAllocator.containsAddress(p)) {
            m_coalesceAllocator.free(p);
            return;
        }

        VirtualAllocPage* page = m_virtualAllocHead;
        while (page) {
            if ((BYTE*)page + sizeof(VirtualAllocPage) == (BYTE*)p) {
                if (page->next) page->next->prev = page->prev;
                if (page->prev) page->prev->next = page->next;
                else m_virtualAllocHead = page->next;
                VirtualFree(page, 0, MEM_RELEASE);
                return;
            }
            page = page->next;
        }
        ASSERT(false);
    }

#ifdef DEBUG
    void CompositeMemoryAllocator::dumpStat() const {
        printf("----------------[DUMP STAT REPORT]----------------\n");
        for (auto &fsa : m_fixedSizeAllocators) {
            printf("----------(FSA %d stat report)----------\n", fsa.getBlockSize());
            FixedSizeAllocator::StatReport fsaStat = fsa.getStatReport();
            printf("Pages: %u\tFree blocks: %u\t Alloc calls: %u\t Free calls: %u\n",
                   fsaStat.pagesCount, fsaStat.freeBlockCount, fsaStat.allocCallCount, fsaStat.freeCallCount);
            printf("----------------------------------------------\n");
        }

        CoalesceAllocator::StatReport coalesceStat = m_coalesceAllocator.getStat();
        printf("---------(Coalesce stat report)---------\n");
        printf("Pages: %u\tTotal alloc size: %u\tAlloc calls: %u\t Free calls: %u\n",
               coalesceStat.pagesCount, coalesceStat.totalAllocSize, coalesceStat.allocCallCount, coalesceStat.freeCallCount);
        printf("----------------------------------------------\n");

        uint32 virtualAllocPages = 0;
        VirtualAllocPage* page = m_virtualAllocHead;
        while (page) page = page->next, virtualAllocPages++;

        printf("---------(Virtual Alloc stat report)--------\n");
        printf("Pages: %u\n", virtualAllocPages);
        printf("----------------------------------------------\n");
        printf("-------------[END DUMP STAT REPORT]---------------\n");
    }

    void CompositeMemoryAllocator::dumpBlocks() const {
        printf("-------------[DUMP ALLOC BLOCKS REPORT]-------------\n");
        for (auto &fsa : m_fixedSizeAllocators) {
            printf("--------(FSA %d alloc blocks report)--------\n", fsa.getBlockSize());
            uint32 pages = fsa.getStatReport().pagesCount;
            for (int i = 0; i < pages; ++i) {
                FixedSizeAllocator::AllocBlocksReport fsaBlocks = fsa.getAllocBlocksReport(i);
                printf("Page %u, alloc count: %u\n", i, fsaBlocks.count);
                for (int j = 0; j < fsaBlocks.count; ++j) {
                    printf("\t%p\n", fsaBlocks.blocks[j]);
                }
            }
            printf("----------------------------------------------\n");
        }

        printf("------------(Coalesce alloc blocks report)------------\n");
        uint32 coalescePages = m_coalesceAllocator.getStat().pagesCount;
        for (int i = 0; i < coalescePages; ++i) {
            printf("Page: %u\n", i);
            CoalesceAllocator::BlockReport report{};
            uint32 count = 0;
            while ((report = m_coalesceAllocator.getNextBlock(i, report.address)).address) {
                if (report.allocated) {
                    printf("\t%p\tsize: %u\n", report.address, report.size);
                    count++;
                }
            }
            printf("Total count on page %u: %u\n", i, count);
        }
        printf("----------------------------------------------\n");
        printf("-----------[END DUMP ALLOC BLOCKS REPORT]-----------\n");
    }

#endif
}
