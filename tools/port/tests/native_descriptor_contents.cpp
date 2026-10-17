#include "gfx_vulkan/bind_cache.h"
#include <cstdlib>
#include <cstdio>
#include <type_traits>
using bo1::vulkan::BindCache;
template<class T>T Handle(std::uintptr_t n){if constexpr(std::is_pointer_v<T>)return reinterpret_cast<T>(n);else return static_cast<T>(n);}
void Check(bool ok){if(!ok)std::abort();}
int main(){
 BindCache::DescriptorContents uniform{Handle<VkBuffer>(1),16};
 auto changed=uniform;Check(changed==uniform);changed.buffer=Handle<VkBuffer>(2);Check(!(changed==uniform));
 changed=uniform;changed.range=32;Check(!(changed==uniform));
 BindCache::DescriptorContents image{VK_NULL_HANDLE,0,Handle<VkImageView>(3),Handle<VkSampler>(4),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
 changed=image;Check(changed==image);changed.imageView=Handle<VkImageView>(5);Check(!(changed==image));
 changed=image;changed.sampler=Handle<VkSampler>(6);Check(!(changed==image));
 changed=image;changed.imageLayout=VK_IMAGE_LAYOUT_GENERAL;Check(!(changed==image));
 BindCache cache;Check(!cache.recentDescriptorStages[0].set&&!cache.recentDescriptorStages[1].set);
 cache.recentDescriptorStages[0].identity=std::make_shared<const char>(0);
 auto held=cache.recentDescriptorStages[0].identity;
 cache=BindCache{};Check(!cache.recentDescriptorStages[0].identity&&held.use_count()==1);
 std::puts("native descriptor contents comparisons passed");
}
