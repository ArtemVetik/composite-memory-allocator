#pragma once

#include "CompositeMemoryAllocator.h"

#include <memory>

namespace MemoryAllocator {
    struct CompositeMemoryAllocatorSingleton {
        inline static std::unique_ptr<CompositeMemoryAllocator::CompositeMemoryAllocator> allocator = nullptr;

        static void init() {
            if (!allocator)
            {
                allocator = std::make_unique<CompositeMemoryAllocator::CompositeMemoryAllocator>();
                allocator->init();
            }
        }
    };

    template <typename T>
    struct MemoryAllocatorT {
        using value_type = T;

        MemoryAllocatorT()
        {
            CompositeMemoryAllocatorSingleton::init();
        }

        ~MemoryAllocatorT() = default;

        template <typename U>
        MemoryAllocatorT(const MemoryAllocatorT<U>& other) noexcept
        {
        }

        template <typename U>
        MemoryAllocatorT(MemoryAllocatorT<U>&& other)
        {
        }

        template <typename U>
        MemoryAllocatorT& operator = (const MemoryAllocatorT<U>& rhs) = delete;
        template <typename U>
        MemoryAllocatorT& operator = (MemoryAllocatorT<U>&& lhs) = delete;

        T* allocate(std::size_t n) {
            if (auto p = CompositeMemoryAllocatorSingleton::allocator->alloc(n * sizeof(T)))
                return static_cast<T*>(p);

            throw std::bad_alloc{};
        }

        void deallocate(T* p, std::size_t) {
            CompositeMemoryAllocatorSingleton::allocator->free(p);
        }

        template <typename U>
        struct rebind {
            using other = MemoryAllocatorT<U>;
        };
    };
}