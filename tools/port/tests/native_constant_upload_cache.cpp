#include "gfx_vulkan/bind_cache.h"
#include <cstdio>
#include <cstdlib>
#include <type_traits>

namespace {
void Require(bool value,const char* reason) {
    if(!value){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}
}
template<class T> T Handle(std::uintptr_t value) {
    if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}
}
int main() {
    using bo1::vulkan::BindCache;
    int storage{},otherStorage{};
    std::array<unsigned char,64> values{};
    BindCache cache;
    VkDescriptorBufferInfo first{Handle<VkBuffer>(1),256,32};
    Require(!cache.constantUploads[0].Matches(&storage,values.data(),32),"empty cache misses");
    cache.constantUploads[0].Remember(&storage,first,values.data());
    Require(cache.constantUploads[0].Matches(&storage,values.data(),32),"unchanged stage hits");
    Require(!cache.constantUploads[1].Matches(&storage,values.data(),32),"stage identity is independent");
    values[48]=7;
    Require(cache.constantUploads[0].Matches(&storage,values.data(),32),"unused rows do not invalidate");
    values[16]=1;
    Require(!cache.constantUploads[0].Matches(&storage,values.data(),32),"used row changes invalidate");
    values[16]=0;
    Require(!cache.constantUploads[0].Matches(&storage,values.data(),16),"smaller range misses");
    Require(!cache.constantUploads[0].Matches(&storage,values.data(),64),"larger range misses");
    Require(!cache.constantUploads[0].Matches(&otherStorage,values.data(),32),"different storage misses");
    // A page switch does not invalidate an earlier immutable, frame-owned range.
    VkDescriptorBufferInfo second{Handle<VkBuffer>(2),0,64};
    cache.constantUploads[1].Remember(&storage,second,values.data());
    Require(cache.constantUploads[0].Matches(&storage,values.data(),32),"old page range remains reusable");
    Require(cache.constantUploads[0].buffer.buffer==first.buffer&&cache.constantUploads[0].buffer.offset==256,
        "cache preserves exact allocation identity");
    Require(cache.constantUploads[1].Matches(&storage,values.data(),64),"second stage has independent range");
    // Production destroys the recording-scope cache before resetting storage.
    cache=BindCache{};
    Require(!cache.constantUploads[0].Matches(&storage,values.data(),32),"new frame cannot reuse old handles");
    Require(!cache.constantUploads[1].Matches(&storage,values.data(),64),"both stage ranges reset");
    std::puts("native constant upload cache tests passed");
}
