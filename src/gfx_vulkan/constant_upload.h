#pragma once
#include "bind_cache.h"
#include "draw_storage.h"
#include <limits>
namespace bo1::vulkan {
// Zero size means the stage has no constants. Cache lifetime is one recording
// scope, ending before DrawStorage resets its fence-retired pages.
inline bool UploadConstantRanges(DrawStorage& storage,VkDeviceSize alignment,const void* const data[2],
    const VkDeviceSize size[2],BindCache* cache,VkDescriptorBufferInfo ranges[2],VkDescriptorPool& pool,std::string& error) {
    ranges[0]={};ranges[1]={};pool=VK_NULL_HANDLE;
    if(!alignment||alignment>std::numeric_limits<VkDeviceSize>::max()-8191) {error="Invalid constant upload alignment";return false;}
    bool reused[2]{};
    for(unsigned stage=0;stage<2;++stage) {
        if(size[stage]>4096||(size[stage]%16)||(size[stage]&&!data[stage])) {error="Invalid constant upload stage range";return false;}
        if(size[stage]&&cache&&cache->reuseConstants)
            reused[stage]=cache->constantUploads[stage].Matches(&storage,data[stage],size[stage]);
    }
    const auto first=size[0]&&!reused[0]?((size[0]+alignment-1)/alignment)*alignment:0;
    const VkDeviceSize bytes=first+(reused[1]?0:size[1]);
    // All-hit draws still reserve descriptor capacity on the current page.
    DrawStorage::Allocation allocation;if(!storage.Allocate(bytes,alignment,allocation,error))return false;
    VkDeviceSize offset=allocation.offset;
    for(unsigned stage=0;stage<2;++stage)if(size[stage]) {
        if(reused[stage])ranges[stage]=cache->constantUploads[stage].buffer;
        else {
            if(!allocation.buffer->Write(offset,data[stage],size[stage],error))return false;
            ranges[stage]={allocation.buffer->Handle(),offset,size[stage]};
        }
        if(!stage)offset+=first;
    }
    for(unsigned stage=0;stage<2;++stage)if(size[stage]&&cache) {
        if(reused[stage]){++cache->constantUploadHits;cache->constantUploadBytesReused+=size[stage];}
        else {
            if(cache->reuseConstants)cache->constantUploads[stage].Remember(&storage,ranges[stage],data[stage]);
            ++cache->constantUploadMisses;
        }
    }
    pool=allocation.descriptors;return true;
}
}
