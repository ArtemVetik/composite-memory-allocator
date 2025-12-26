#include "lib/googletest/include/gtest/gtest.h"

#include <FixedSizeAllocator.h>

namespace FixedSizeAllocator {
    void AllocateRange(FixedSizeAllocator &allocator, std::vector<void*> &plist, int count, uint32 size) {
        for (int i = 0; i < count; i++) {
            void* p = (int*)allocator.alloc(size);
            EXPECT_TRUE(p != nullptr);
            plist.push_back(p);
        }
    }

    void FreeRangeRandom(FixedSizeAllocator &allocator, std::vector<void*> &plist, int count) {
        for (int i = 0; i < count; i++) {
            int index = rand() % plist.size();
            allocator.free(plist[index]);
            plist.erase(plist.begin() + index);
        }
    }

    TEST(FSA, FSA16)
    {
        FixedSizeAllocator fsa;
        fsa.init(16);
        int* p = (int*)fsa.alloc(4 * sizeof(int));
        EXPECT_TRUE(p != nullptr);
        fsa.free(p);
        fsa.destroy();
    }

    TEST(FSA, FSA32)
    {
        FixedSizeAllocator fsa;
        fsa.init(32);
        int* p = (int*)fsa.alloc(8 * sizeof(int));
        EXPECT_TRUE(p != nullptr);
        fsa.free(p);
        fsa.destroy();
    }

    TEST(FSA, FSA64)
    {
        FixedSizeAllocator fsa;
        fsa.init(64);
        int* p = (int*)fsa.alloc(16 * sizeof(int));
        EXPECT_TRUE(p != nullptr);
        fsa.free(p);
        fsa.destroy();
    }

    TEST(FSA, FSA128)
    {
        FixedSizeAllocator fsa;
        fsa.init(128);
        int* p = (int*)fsa.alloc(32 * sizeof(int));
        EXPECT_TRUE(p != nullptr);
        fsa.free(p);
        fsa.destroy();
    }

    TEST(FSA, FSA256)
    {
        FixedSizeAllocator fsa;
        fsa.init(256);
        int* p = (int*)fsa.alloc(64 * sizeof(int));
        EXPECT_TRUE(p != nullptr);
        fsa.free(p);
        fsa.destroy();
    }

    TEST(FSA, FSA512)
    {
        FixedSizeAllocator fsa;
        fsa.init(512);
        int* p = (int*)fsa.alloc(128 * sizeof(int));
        EXPECT_TRUE(p != nullptr);
        fsa.free(p);
        fsa.destroy();
    }

    TEST(FSA, DoubleFree)
    {
        FixedSizeAllocator fsa;
        fsa.init(512);
        int* p = (int*)fsa.alloc(128 * sizeof(int));
        EXPECT_TRUE(p != nullptr);
        fsa.free(p);
        EXPECT_DEATH(fsa.free(p), "");
        fsa.destroy();
    }

    TEST(FSA, MemoryLeak)
    {
        FixedSizeAllocator fsa;
        fsa.init(512);
        int* p = (int*)fsa.alloc(128 * sizeof(int));
        EXPECT_TRUE(p != nullptr);
        EXPECT_DEATH(fsa.destroy(), "");
        fsa.free(p);
        fsa.destroy();
    }

    TEST(FSA, FewPages)
    {
        FixedSizeAllocator fsa;
        fsa.init(16);
        int* p[4099];
        for (int i = 0; i < 4099; i++) {
            p[i] = (int*)fsa.alloc(2 * sizeof(int));
            EXPECT_TRUE(p[i] != nullptr);
        }
        for (int i = 0; i < 4099; i++) {
            fsa.free(p[i]);
        }
        fsa.destroy();
    }

    TEST(FSA, AllocAndFreeRandom)
    {
        FixedSizeAllocator fsa;
        fsa.init(256);

        std::vector<void*> plist;
        AllocateRange(fsa, plist, 300, 200);
        FreeRangeRandom(fsa, plist, 50);
        AllocateRange(fsa, plist, 400, 200);
        FreeRangeRandom(fsa, plist, 350);
        AllocateRange(fsa, plist, 50, 200);
        FreeRangeRandom(fsa, plist, 250);
        AllocateRange(fsa, plist, 50, 200);
        FreeRangeRandom(fsa, plist, 150);

        fsa.destroy();
    }
}