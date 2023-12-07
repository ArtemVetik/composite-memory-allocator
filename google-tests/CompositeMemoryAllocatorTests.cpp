#include "lib/googletest/include/gtest/gtest.h"
#include "../scr/CompositeMemoryAllocator.cpp"

namespace CompositeMemoryAllocator {
    void AllocateRange(CompositeMemoryAllocator &allocator, std::vector<void*> &plist, int count, uint32 minSize, uint32 maxSize) {
        for (int i = 0; i < count; i++) {
            void* p = (int*)allocator.alloc(minSize + rand() % maxSize);
            EXPECT_TRUE(p != nullptr);
            plist.push_back(p);
        }
    }

    void FreeRangeRandom(CompositeMemoryAllocator &allocator, std::vector<void*> &plist, int count) {
        for (int i = 0; i < count; i++) {
            int index = rand() % plist.size();
            allocator.free(plist[index]);
            plist.erase(plist.begin() + index);
        }
    }

    TEST(CompositeMemoryAllocator, SimpleAllocAndFree) {
        CompositeMemoryAllocator allocator;
        allocator.init();

        void* p[10];
        for (int i = 1; i <= 10; i++) {
            p[i-1] = allocator.alloc(i * i * i * i);
            EXPECT_TRUE(p[i-1] != nullptr);
        }
        for (int i = 1; i <= 10; i++)
            allocator.free(p[i-1]);

        allocator.destroy();
    }

    TEST(CompositeMemoryAllocator, MemoryLeak) {
        CompositeMemoryAllocator allocator;
        allocator.init();
        void* p = allocator.alloc(12 * 1024 * 1024);
        EXPECT_DEATH(allocator.destroy(), "");
        allocator.free(p);
        allocator.destroy();
    }

    TEST(CompositeMemoryAllocator, DoubleFree) {
        CompositeMemoryAllocator allocator;
        allocator.init();
        void* p = allocator.alloc(11 * 1024 * 1024);
        allocator.free(p);
        EXPECT_DEATH(allocator.free(p), "");
        allocator.destroy();
    }

    TEST(CompositeMemoryAllocator, SmallAndBigAlloc) {
        CompositeMemoryAllocator allocator;
        allocator.init();
        void* p1 = allocator.alloc(2);
        EXPECT_TRUE(p1 != nullptr);
        void* p2 = allocator.alloc(16 * 1024 * 1024);
        EXPECT_TRUE(p2 != nullptr);
        allocator.free(p2);
        allocator.free(p1);
        allocator.destroy();
    }

    TEST(CompositeMemoryAllocator, RandomAllocAndFree) {
        CompositeMemoryAllocator allocator;
        allocator.init();

        std::vector<void*> plist;
        for(int i = 0; i < 1000; i++) {
            AllocateRange(allocator, plist, 1 + rand() % 1000, 1, 1024 * 1024);
            FreeRangeRandom(allocator, plist, rand() % plist.size());
        }

        for(auto &p : plist)
            allocator.free(p);

        allocator.destroy();
    }
}