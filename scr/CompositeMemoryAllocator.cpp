#include "../include/CompositeMemoryAllocator.h"

#include <windows.h>

namespace CompositeMemoryAllocator {

    void CompositeMemoryAllocatorTests::CompositeMemoryAllocatorTests::init() {
        for (int i = 0; i < BLOCK_TYPE_COUNT; ++i) {
            m_fixedSizeAllocators[i].init(1 << (FSABlockSize::FSA16 + i));
        }

        m_coalesceAllocator.init();
    }

    void CompositeMemoryAllocatorTests::CompositeMemoryAllocatorTests::destroy() {
        for (auto & m_fixedSizeAllocator : m_fixedSizeAllocators)
            m_fixedSizeAllocator.destroy();

        m_coalesceAllocator.destroy();
    }

    void* CompositeMemoryAllocatorTests::CompositeMemoryAllocatorTests::alloc(uint32 size) {
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
            return page;
        }
    }

    void CompositeMemoryAllocatorTests::CompositeMemoryAllocatorTests::free(void *p) {
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
                VirtualFree(page, 0, MEM_RELEASE);
                if (page->next) page->next->prev = page->prev;
                if (page->prev) page->prev->next = page->next;
                else m_virtualAllocHead = page->next;
                return;
            }
            page = page->next;
        }
    }
}
