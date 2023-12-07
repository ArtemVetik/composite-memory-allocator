#include <iostream>
#include <vector>

#include "../include/CompositeMemoryAllocator.h"

void AllocateRange(CompositeMemoryAllocator::CompositeMemoryAllocator &allocator, std::vector<int*> &plist, int count, unsigned int minSize, unsigned int  maxSize) {
    for (int i = 0; i < count; i++) {
        int* p = (int*)allocator.alloc(minSize + rand() % maxSize);
        plist.push_back(p);
    }
}

void FreeRangeRandom(CompositeMemoryAllocator::CompositeMemoryAllocator &allocator, std::vector<int*> &plist, int count) {
    for (int i = 0; i < count; i++) {
        int index = rand() % plist.size();
        allocator.free(plist[index]);
        plist.erase(plist.begin() + index);
    }
}

int main() {
    srand((unsigned int)time(nullptr));
    CompositeMemoryAllocator::CompositeMemoryAllocator allocator;
    allocator.init();

    std::vector<int*> plist;
    for(int i = 0; i < 100; i++) {
        AllocateRange(allocator, plist, 1 + rand() % 1000, 1, 1024);
        FreeRangeRandom(allocator, plist, rand() % plist.size());
    }

    allocator.dumpStat();
    printf("\n\n");
    allocator.dumpBlocks();

    for(auto &p : plist)
        allocator.free(p);

    allocator.destroy();

    return 0;
}