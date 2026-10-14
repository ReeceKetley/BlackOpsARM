#include "aligned_memory.h"
#include <cstdlib>
#include <cerrno>
#include <algorithm>
void *bo1::android::AllocateAligned(std::size_t bytes,std::size_t alignment) {
    if (!bytes || !alignment || (alignment&(alignment-1))) { errno=EINVAL; return nullptr; }
    void *memory=nullptr;
    const int error=posix_memalign(&memory,std::max(alignment,sizeof(void *)),bytes);
    if(error) { errno=error; return nullptr; }
    return memory;
}
void bo1::android::FreeAligned(void *memory) { std::free(memory); }
