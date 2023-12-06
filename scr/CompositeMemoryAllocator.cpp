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
            //int index = __builtin_clz((size - 1) >> 3) ^ 31;
            //m_fixedSizeAllocators[index].alloc(size);

            for (auto &fsa : m_fixedSizeAllocators)
                if (fsa.getBlockSize() >= size)
                    return fsa.alloc(size);
        }
        else if (size <= CoalesceAllocator::PAGE_SIZE) {
            return m_coalesceAllocator.alloc(size);
        }
        else {
            return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        }
    }

    void CompositeMemoryAllocatorTests::CompositeMemoryAllocatorTests::free(void *p) {
        for (auto &fsa : m_fixedSizeAllocators) {
            if (fsa.containsAddress(p)) {
                fsa.free(p);
                return;
            }
        }

        if (m_coalesceAllocator.containsAddress(p))
            m_coalesceAllocator.free(p);

        //TODO: else VirtualFree(...)
    }
}
