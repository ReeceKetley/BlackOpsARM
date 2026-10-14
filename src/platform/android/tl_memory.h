#pragma once
#include <cstddef>
struct tlSystemCallbacks;
namespace bo1::android {
void *TLAllocate(unsigned size,unsigned alignment,unsigned flags,const tlSystemCallbacks &callbacks);
void *TLReallocate(void *pointer,unsigned size,unsigned alignment,unsigned flags);
void TLFree(void *pointer);
std::size_t TLAllocationCount();
}
