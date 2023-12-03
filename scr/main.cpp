#include "../include/FixedSizeAllocator.h"

int main() {
    FixedSizeAllocator fsa64(FSABlockSize::kFSA64);
    fsa64.init();

    int* pi = (int*)fsa64.alloc(sizeof(int));
    double *pd = (double*)fsa64.alloc(sizeof(double));
    int *pa = (int*)fsa64.alloc(10 * sizeof(int));

    fsa64.dumpStat();
    fsa64.dumpBlocks();

    fsa64.free(pa);
    fsa64.free(pd);
    fsa64.free(pi);

    fsa64.destroy();

    return 0;
}
