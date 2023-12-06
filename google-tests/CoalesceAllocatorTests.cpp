#include "lib/googletest/include/gtest/gtest.h."
#include "../scr/CoalesceAllocator.cpp"

namespace CoalesceAllocator {
    void AllocateRange(CoalesceAllocator &allocator, std::vector<void*> &plist, int count, uint32 minSize, uint32 maxSize) {
        for (int i = 0; i < count; i++) {
            void* p = (int*)allocator.alloc(minSize + rand() % maxSize);
            EXPECT_TRUE(p != nullptr);
            plist.push_back(p);
        }
    }

    void FreeRangeRandom(CoalesceAllocator &allocator, std::vector<void*> &plist, int count) {
        for (int i = 0; i < count; i++) {
            int index = rand() % plist.size();
            allocator.free(plist[index]);
            plist.erase(plist.begin() + index);
        }
    }

    TEST(CoalesceAllocator, SingleAllocate)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();
        void* p = allocator.alloc(2 * 4096);
        allocator.free(p);
        allocator.destroy();
    }

    TEST(CoalesceAllocator, SmallBlocks)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();
        void* p1 = allocator.alloc(128);
        void* p2 = allocator.alloc(64);
        EXPECT_TRUE(p1 != nullptr);
        EXPECT_TRUE(p2 != nullptr);
        allocator.free(p2);
        allocator.free(p1);
        allocator.destroy();
    }

    TEST(CoalesceAllocator, DoubleFree)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();
        void* p = allocator.alloc(1024);
        EXPECT_TRUE(p != nullptr);
        allocator.free(p);
        ASSERT_DEATH(allocator.free(p), "");
        allocator.destroy();
    }

    TEST(CoalesceAllocator, MemoryLeak)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();
        void* p = allocator.alloc(1024);
        ASSERT_DEATH(allocator.destroy(), "");
        allocator.free(p);
        allocator.destroy();
    }

    TEST(CoalesceAllocator, MaxSize)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();
        void* p1 = allocator.alloc(PAGE_SIZE);
        void* p2 = allocator.alloc(PAGE_SIZE);
        void* p3 = allocator.alloc(PAGE_SIZE);
        EXPECT_TRUE(p1 != nullptr);
        EXPECT_TRUE(p2 != nullptr);
        EXPECT_TRUE(p3 != nullptr);
        allocator.free(p3);
        allocator.free(p2);
        allocator.free(p1);
        allocator.destroy();
    }

    TEST(CoalesceAllocator, AllocAndFreeContinuously)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();
        int* p[512];

        for (int i = 0; i < 512; i++) {
            p[i] = (int*)allocator.alloc(10 * 4096);
            EXPECT_TRUE(p[i] != nullptr);
        }
        for (int i = 0; i < 512; i++) {
            allocator.free(p[i]);
        }

        allocator.destroy();
    }

    TEST(CoalesceAllocator, AllocAndFreeRandom)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();
        std::vector<void*> plist;

        AllocateRange(allocator, plist, 1024, 4096, 10*4096);
        FreeRangeRandom(allocator, plist, 1024);

        allocator.destroy();
    }

    TEST(CoalesceAllocator, AllocAndFreeFewTimesRandom)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();
        std::vector<void*> plist;

        AllocateRange(allocator, plist, 512, 4096, 10*4096);
        FreeRangeRandom(allocator, plist, 256);
        AllocateRange(allocator, plist, 200, 4096, 10*4096);
        FreeRangeRandom(allocator, plist, 256);
        AllocateRange(allocator, plist, 100, 4096, 10*4096);
        FreeRangeRandom(allocator, plist, 300);
        allocator.destroy();
    }

    TEST(CoalesceAllocator, AllocSplit)
    {
        CoalesceAllocator allocator = CoalesceAllocator();
        allocator.init();

        void* p = allocator.alloc(PAGE_SIZE - 61);
        allocator.free(p);

        allocator.destroy();
    }
}