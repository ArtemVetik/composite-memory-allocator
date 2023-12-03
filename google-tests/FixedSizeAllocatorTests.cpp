#include "lib/googletest/include/gtest/gtest.h."
#include "../scr/FixedSizeAllocator.cpp"

TEST(FSAAllocTest, FSA16)
{
FixedSizeAllocator fsa(FSABlockSize::kFSA16);
fsa.init();
int* p = (int*)fsa.alloc(4 * sizeof(int));
EXPECT_TRUE(p != nullptr);
fsa.free(p);
fsa.destroy();
}

TEST(FSAAllocTest, FSA32)
{
FixedSizeAllocator fsa(FSABlockSize::kFSA32);
fsa.init();
int* p = (int*)fsa.alloc(8 * sizeof(int));
EXPECT_TRUE(p != nullptr);
fsa.free(p);
fsa.destroy();
}

TEST(FSAAllocTest, FSA64)
{
FixedSizeAllocator fsa(FSABlockSize::kFSA64);
fsa.init();
int* p = (int*)fsa.alloc(16 * sizeof(int));
EXPECT_TRUE(p != nullptr);
fsa.free(p);
fsa.destroy();
}

TEST(FSAAllocTest, FSA128)
{
FixedSizeAllocator fsa(FSABlockSize::kFSA128);
fsa.init();
int* p = (int*)fsa.alloc(32 * sizeof(int));
EXPECT_TRUE(p != nullptr);
fsa.free(p);
fsa.destroy();
}

TEST(FSAAllocTest, FSA256)
{
FixedSizeAllocator fsa(FSABlockSize::kFSA256);
fsa.init();
int* p = (int*)fsa.alloc(64 * sizeof(int));
EXPECT_TRUE(p != nullptr);
fsa.free(p);
fsa.destroy();
}

TEST(FSAAllocTest, FSA512)
{
FixedSizeAllocator fsa(FSABlockSize::kFSA512);
fsa.init();
int* p = (int*)fsa.alloc(128 * sizeof(int));
EXPECT_TRUE(p != nullptr);
fsa.free(p);
fsa.destroy();
}

TEST(FSAAllocTest, FewPages)
{
FixedSizeAllocator fsa(FSABlockSize::kFSA16);
fsa.init();
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