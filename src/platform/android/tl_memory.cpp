#include "tl_memory.h"
#include <tl/tl_system.h>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <unordered_map>
namespace {
struct Owner { unsigned bytes, alignment; tlSystemCallbacks callbacks; };
std::mutex &ownerMutex() { static auto *value=new std::mutex;return *value; }
auto &owners() { static auto *value=new std::unordered_map<void *,Owner>;return *value; }
[[noreturn]] void fail(const char *reason) {std::fprintf(stderr,"Native TL memory: %s\n",reason);std::abort();}
void *allocate(unsigned size,unsigned alignment,unsigned flags,const tlSystemCallbacks &callbacks) {
    if(callbacks.MemAlloc) {
        if(!callbacks.MemFree)fail("allocator callback requires matching free callback");
        return callbacks.MemAlloc(size,alignment,flags);
    }
    void *result=nullptr;
    const std::size_t actual=std::max<std::size_t>(alignment?alignment:16,sizeof(void *));
    if((actual&(actual-1)) || posix_memalign(&result,actual,size?size:1))return nullptr;
    return result;
}
void release(void *pointer,const Owner &owner) {
    if(owner.callbacks.MemFree)owner.callbacks.MemFree(pointer);
    else std::free(pointer);
}
}
void *bo1::android::TLAllocate(unsigned size,unsigned alignment,unsigned flags,const tlSystemCallbacks &callbacks) {
    auto provider=callbacks;
    if(!provider.MemAlloc){provider.MemFree=nullptr;provider.MemRealloc=nullptr;}
    void *pointer=allocate(size,alignment,flags,callbacks);
    if(!pointer)return nullptr;
    try {
        std::lock_guard<std::mutex> lock(ownerMutex());
        if(!owners().emplace(pointer,Owner{size,alignment,provider}).second)fail("duplicate live allocation");
    } catch (...) {release(pointer,Owner{size,alignment,provider});return nullptr;}
    return pointer;
}
void bo1::android::TLFree(void *pointer) {
    if(!pointer)return;
    Owner owner;
    {
        std::lock_guard<std::mutex> lock(ownerMutex());
        auto found=owners().find(pointer);
        if(found==owners().end())fail("free of unknown allocation");
        owner=found->second;owners().erase(found);
    }
    release(pointer,owner);
}
void *bo1::android::TLReallocate(void *pointer,unsigned size,unsigned alignment,unsigned flags) {
    if(!pointer)fail("reallocation requires allocation owner");
    Owner owner;
    {
        std::lock_guard<std::mutex> lock(ownerMutex());
        auto found=owners().find(pointer);if(found==owners().end())fail("realloc of unknown allocation");
        owner=found->second;
    }
    // Keep the allocating provider even if global callbacks changed afterwards.
    if(owner.callbacks.MemRealloc) {
        void *next=owner.callbacks.MemRealloc(pointer,size,alignment,flags);
        if(!next)return nullptr;
        std::lock_guard<std::mutex> lock(ownerMutex());
        auto node=owners().extract(pointer);if(node.empty())fail("allocation owner lost during realloc");
        node.key()=next;node.mapped().bytes=size;node.mapped().alignment=alignment;
        if(!owners().insert(std::move(node)).inserted)fail("realloc collides with live allocation");
        return next;
    }
    void *next=TLAllocate(size,alignment,flags,owner.callbacks);
    if(!next)return nullptr; // Failure preserves the original pointer and data.
    std::memcpy(next,pointer,std::min(size,owner.bytes));TLFree(pointer);return next;
}
std::size_t bo1::android::TLAllocationCount() {
    std::lock_guard<std::mutex> lock(ownerMutex());return owners().size();
}
