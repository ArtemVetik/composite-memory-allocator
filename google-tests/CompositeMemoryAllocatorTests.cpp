#include "lib/googletest/include/gtest/gtest.h"
#include "../scr/CompositeMemoryAllocator.cpp"

namespace CompositeMemoryAllocator {
    void AllocateRange(CompositeMemoryAllocatorTests &allocator, std::vector<void*> &plist, int count, uint32 minSize, uint32 maxSize) {
        for (int i = 0; i < count; i++) {
            void* p = (int*)allocator.alloc(minSize + rand() % maxSize);
            EXPECT_TRUE(p != nullptr);
            plist.push_back(p);
        }
    }

    void FreeRangeRandom(CompositeMemoryAllocatorTests &allocator, std::vector<void*> &plist, int count) {
        for (int i = 0; i < count; i++) {
            int index = rand() % plist.size();
            allocator.free(plist[index]);
            plist.erase(plist.begin() + index);
        }
    }

    TEST(CompositeMemoryAllocator, SimpleAllocAndFree) {
        CompositeMemoryAllocatorTests allocator;
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
        CompositeMemoryAllocatorTests allocator;
        allocator.init();
        void* p1 = allocator.alloc(100);
        void* p2 = allocator.alloc(1000);
        EXPECT_DEATH(allocator.destroy(), "");
        allocator.free(p2);
        EXPECT_DEATH(allocator.destroy(), "");
        allocator.free(p1);
        allocator.destroy();
    }

    TEST(CompositeMemoryAllocator, DoubleFree) {
        CompositeMemoryAllocatorTests allocator;
        allocator.init();
        void* p1 = allocator.alloc(100);
        void* p2 = allocator.alloc(1000);
        allocator.free(p2);
        EXPECT_DEATH(allocator.free(p2), "");
        allocator.free(p1);
        EXPECT_DEATH(allocator.free(p1), "");
        allocator.destroy();
    }

    TEST(CompositeMemoryAllocator, SmallAndBigAlloc) {
        CompositeMemoryAllocatorTests allocator;
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
        CompositeMemoryAllocatorTests allocator;
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