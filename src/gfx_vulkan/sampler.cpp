#include "texture_bindings.h"
#include <cmath>
#include <map>
#include <tuple>
#include <mutex>
#include <cstring>
namespace bo1::vulkan::texture_bindings {
namespace {
bool Address(unsigned code,VkSamplerAddressMode& result) {
    switch(code) {case 1:result=VK_SAMPLER_ADDRESS_MODE_REPEAT;return true;
    case 2:result=VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;return true;
    case 3:result=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;return true;default:return false;}
}
}
std::shared_ptr<Sampler> Sampler::Acquire(VkDevice device,unsigned decoded,unsigned mips,float bias,std::string& error,bool comparison) {
    if(!device||!mips){error="Invalid native cached sampler device/mip count";return {};}
    if(!Validate(decoded,bias,error))return {};
    // Exact input key includes device and comparison policy. The weak owner
    // cannot return a destroyed sampler or retain a sampler past device drain.
    using Key=std::tuple<VkDevice,unsigned,unsigned,std::uint32_t,bool>;
    static std::mutex cacheMutex;static std::map<Key,std::weak_ptr<Sampler>> cache;
    std::uint32_t bits{};std::memcpy(&bits,&bias,sizeof(bits));
    const Key key{device,decoded,mips,bits,comparison};
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto found=cache.find(key);if(found!=cache.end())if(auto result=found->second.lock())return result;
    auto result=std::make_shared<Sampler>(device);
    if(!result->Create(decoded,mips,bias,error,comparison))return {};
    // Bound metadata without affecting actual captured/GPU users. Eviction
    // only removes a lookup entry; shared owners still retire at their fences.
    if(cache.size()>=256){for(auto i=cache.begin();i!=cache.end();)if(i->second.expired())i=cache.erase(i);else ++i;}
    if(cache.size()>=256)cache.erase(cache.begin());
    cache[key]=result;return result;
}
Sampler::~Sampler(){if(sampler_)vkDestroySampler(device_,sampler_,nullptr);}
bool Sampler::Validate(unsigned decoded,float bias,std::string& error) {
    const unsigned min=(decoded>>8)&15,mag=(decoded>>12)&15,mip=(decoded>>16)&15;
    VkSamplerAddressMode address{};
    if(!std::isfinite(bias)||min<1||min>3||mag<1||mag>3||mip>2||(decoded&0xfc000000u)
        ||!Address((decoded>>20)&3,address)||!Address((decoded>>22)&3,address)||!Address((decoded>>24)&3,address)) {
        error="Unsupported native decoded sampler state";return false;
    }
    return true;
}
bool Sampler::Create(unsigned decoded,unsigned mips,float bias,std::string& error,bool comparison) {
    if(sampler_||!mips) {error="Invalid native sampler owner/mip count";return false;}
    if(!Validate(decoded,bias,error))return false;
    const unsigned min=(decoded>>8)&15,mag=(decoded>>12)&15,mip=(decoded>>16)&15;
    VkSamplerCreateInfo create{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    create.minFilter=min==1?VK_FILTER_NEAREST:VK_FILTER_LINEAR;
    create.magFilter=mag==1?VK_FILTER_NEAREST:VK_FILTER_LINEAR;
    create.mipmapMode=mip==2?VK_SAMPLER_MIPMAP_MODE_LINEAR:VK_SAMPLER_MIPMAP_MODE_NEAREST;
    create.maxLod=mip?float(mips-1):0;
    create.compareEnable=comparison?VK_TRUE:VK_FALSE;create.compareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
    // Mobile default: anisotropic requests use linear sampling at a 1x cap.
    // Device optional anisotropy is not enabled; do not set its feature flag.
    // Bias is conservatively bounded for Vulkan's guaranteed minimum limit.
    create.mipLodBias=bias<-2?-2:(bias>2?2:bias);
    if(!Address((decoded>>20)&3,create.addressModeU)||!Address((decoded>>22)&3,create.addressModeV)||!Address((decoded>>24)&3,create.addressModeW)) {
        error="Invalid native sampler address mode";return false;
    }
    const auto result=vkCreateSampler(device_,&create,nullptr,&sampler_);
    if(result!=VK_SUCCESS) {error="Native sampler creation: "+std::to_string(int(result));return false;}comparison_=comparison;return true;
}
}
