#include "image_assets.h"
#include "pointer_hash.h"
#include "image_uploads.h"
#include "block_compression.h"
#include "buffer_assets.h"
#include "shader_assets.h"
#include "gfx_d3d/r_material.h"
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <cstring>
#include <algorithm>
#include <limits>
#include <android/log.h>
namespace bo1::vulkan::image_assets {
namespace {
std::mutex mutex;
VkPhysicalDevice physical{};VkDevice device{};std::uint64_t serial{};
std::atomic<bool> nativeBlocks{};
std::unordered_map<const GfxImage*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> images;
std::vector<std::weak_ptr<Texture2D>> liveTextures;
bool Drained() {
    liveTextures.erase(std::remove_if(liveTextures.begin(),liveTextures.end(),[](const auto& weak){return weak.expired();}),liveTextures.end());
    return images.empty()&&liveTextures.empty();
}
bool Format(unsigned format,Record& record,std::string& error) {
    auto& d=record.description;
    record.sourceFormat=format;
    switch(format) {
    case D3DFMT_D16:d.format=VK_FORMAT_D16_UNORM;d.aspects=VK_IMAGE_ASPECT_DEPTH_BIT;record.bytesPerTexel=2;d.linearSampling=false;break;
    case D3DFMT_D24X8:d.format=VK_FORMAT_X8_D24_UNORM_PACK32;d.aspects=VK_IMAGE_ASPECT_DEPTH_BIT;record.bytesPerTexel=4;d.linearSampling=false;break;
    case D3DFMT_D24S8:d.format=VK_FORMAT_D24_UNORM_S8_UINT;d.aspects=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;record.bytesPerTexel=4;d.linearSampling=false;break;
    case D3DFMT_DXT1:case D3DFMT_DXT3:case D3DFMT_DXT5:
        record.sourceBlockBytes=format==D3DFMT_DXT1?8:16;
        if(nativeBlocks.load(std::memory_order_acquire)) {
            // D3D9 DXT1 decodes index 3 (c0<=c1) as transparent black: BC1_RGBA.
            d.format=format==D3DFMT_DXT1?VK_FORMAT_BC1_RGBA_UNORM_BLOCK:(format==D3DFMT_DXT3?VK_FORMAT_BC2_UNORM_BLOCK:VK_FORMAT_BC3_UNORM_BLOCK);
            record.nativeBlocks=true;record.bytesPerTexel=0;break;
        }
        // Device without BC sampling: decode once at publication, never per frame.
        d.format=VK_FORMAT_R8G8B8A8_UNORM;record.bytesPerTexel=4;break;
    case D3DFMT_A8R8G8B8:case D3DFMT_X8R8G8B8:
        d.format=VK_FORMAT_B8G8R8A8_UNORM;record.bytesPerTexel=4;
        if(format==D3DFMT_X8R8G8B8)d.components.a=VK_COMPONENT_SWIZZLE_ONE;break;
    case D3DFMT_A8B8G8R8:d.format=VK_FORMAT_R8G8B8A8_UNORM;record.bytesPerTexel=4;break;
    case D3DFMT_L8:case D3DFMT_A8:
        d.format=VK_FORMAT_R8_UNORM;record.bytesPerTexel=1;
        if(format==D3DFMT_L8)d.components={VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_ONE};
        else d.components={VK_COMPONENT_SWIZZLE_ONE,VK_COMPONENT_SWIZZLE_ONE,VK_COMPONENT_SWIZZLE_ONE,VK_COMPONENT_SWIZZLE_R};break;
    case D3DFMT_A8L8:d.format=VK_FORMAT_R8G8_UNORM;record.bytesPerTexel=2;
        d.components={VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_G};break;
    case D3DFMT_R5G6B5:d.format=VK_FORMAT_R5G6B5_UNORM_PACK16;record.bytesPerTexel=2;break;
    case D3DFMT_G16R16:d.format=VK_FORMAT_R16G16_UNORM;record.bytesPerTexel=4;break;
    case D3DFMT_A16B16G16R16:d.format=VK_FORMAT_R16G16B16A16_UNORM;record.bytesPerTexel=8;break;
    case D3DFMT_A16B16G16R16F:d.format=VK_FORMAT_R16G16B16A16_SFLOAT;record.bytesPerTexel=8;break;
    case D3DFMT_R32F:d.format=VK_FORMAT_R32_SFLOAT;record.bytesPerTexel=4;break;
    default:error="Native sampled image format not implemented: "+std::to_string(format);return false;
    }
    return true;
}
}
bool Empty(){std::lock_guard<std::mutex> lock(mutex);return Drained();}
bool Attach(VkPhysicalDevice nextPhysical,VkDevice nextDevice,std::string& error,bool blockCompression) {
    std::lock_guard<std::mutex> lock(mutex);
    if((!nextPhysical)!=(!nextDevice)) {error="Incomplete native image device context";return false;}
    if(blockCompression&&!nextDevice) {error="Block compression requires an attached device";return false;}
    if(physical==nextPhysical&&device==nextDevice) {
        if(nextDevice&&blockCompression!=nativeBlocks.load(std::memory_order_relaxed)&&!Drained()) {error="Cannot change texture block policy while owners survive";return false;}
        if(!buffer_assets::Attach(nextPhysical,nextDevice,error))return false;
        nativeBlocks.store(blockCompression,std::memory_order_release);return true;
    }
    if(!Drained()) {error="Cannot replace image device while owners/borrowers survive";return false;}
    if(!buffer_assets::Attach(nextPhysical,nextDevice,error))return false;
    physical=nextPhysical;device=nextDevice;nativeBlocks.store(blockCompression,std::memory_order_release);return true;
}
std::uint64_t SubresourceBytes(const Record& record,unsigned mip) {
    const auto& d=record.description;
    const std::uint64_t width=std::max(1u,d.extent.width>>mip),height=std::max(1u,d.extent.height>>mip),depth=std::max(1u,d.extent.depth>>mip);
    if(record.nativeBlocks)return ((width+3)/4)*((height+3)/4)*depth*record.sourceBlockBytes;
    return width*height*depth*record.bytesPerTexel;
}
bool Create(GfxImage* owner,unsigned mipCount,int flags,unsigned sourceFormat,std::string& error) {
    if(!shader_assets::Active()||!owner||owner->texture.basemap) {error="Invalid native image owner/backend";return false;}
    auto record=std::make_shared<Record>();record->createdName=owner->name?owner->name:"<unnamed>";auto& d=record->description;
    d.extent={owner->width,owner->height,owner->depth};
    if(owner->mapType==4) {d.type=VK_IMAGE_TYPE_3D;d.viewType=VK_IMAGE_VIEW_TYPE_3D;}
    else if(owner->mapType==5) {d.viewType=VK_IMAGE_VIEW_TYPE_CUBE;d.arrayLayers=6;}
    else if(owner->mapType!=3) {error="Unknown native image map type";return false;}
    if(!mipCount) {
        auto size=std::max(d.extent.width,std::max(d.extent.height,d.extent.depth));
        for(;size;size>>=1)++mipCount;
    }
    d.mipLevels=mipCount;
    if(!Format(sourceFormat,*record,error))return false;
    if(flags&0x20000) {
        if(d.type!=VK_IMAGE_TYPE_2D||d.arrayLayers!=1||mipCount!=1||record->sourceBlockBytes) {error="Unsupported native attachment shape/format";return false;}
        d.usage|=VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        d.usage|=(d.aspects&VK_IMAGE_ASPECT_DEPTH_BIT)?VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    } else if(d.aspects!=VK_IMAGE_ASPECT_COLOR_BIT) {error="Depth image requires attachment usage";return false;}
    std::lock_guard<std::mutex> lock(mutex);
    if(!shader_assets::Active()||!device||!physical) {error="Native image device/backend unavailable";return false;}
    const auto existing=images.find(owner);
    if(existing!=images.end()) {
        const auto& prior=*existing->second;
        error="Native image owner already registered: created_name="+prior.createdName+" serial="+std::to_string(prior.serial)
            +" revision="+std::to_string(prior.revision)+" old_format="+std::to_string(prior.sourceFormat)
            +" new_format="+std::to_string(sourceFormat)+" old_extent="+std::to_string(prior.description.extent.width)
            +"x"+std::to_string(prior.description.extent.height)+" new_extent="+std::to_string(d.extent.width)+"x"+std::to_string(d.extent.height);
        return false;
    }
    if(sourceFormat==D3DFMT_D24X8) {
        VkImageFormatProperties supported{};
        if(vkGetPhysicalDeviceImageFormatProperties(physical,d.format,d.type,VK_IMAGE_TILING_OPTIMAL,d.usage,0,&supported)!=VK_SUCCESS) {
            // A combined physical format can supply the same 24-bit depth;
            // the original X8 source never exposes or uses the stencil bits.
            d.format=VK_FORMAT_D24_UNORM_S8_UINT;d.aspects|=VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    // Image allocation does not require linear filtering. Record the actual
    // device capability; binding validation rejects an unsupported sampler.
    VkFormatProperties samplingProperties{};
    vkGetPhysicalDeviceFormatProperties(physical,d.format,&samplingProperties);
    d.linearSampling=(samplingProperties.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)!=0;
    record->texture=std::make_shared<Texture2D>(physical,device);
    if(!record->texture->Create(d,error)) {
        __android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","image_create_failed source_format=%u vk_format=%u usage=%u linear=%u error=%s",sourceFormat,unsigned(d.format),unsigned(d.usage),unsigned(d.linearSampling),error.c_str());
        return false;
    }
    record->serial=++serial;
    if(record->serial<=3)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","image_initial_owner serial=%llu owner=%p name=%s extent=%ux%u",(unsigned long long)record->serial,owner,record->createdName.c_str(),d.extent.width,d.extent.height);
    liveTextures.push_back(record->texture);images.emplace(owner,std::move(record));return true;
}
bool EnableDepthComparison(const GfxImage* owner,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);auto it=images.find(owner);
    if(it==images.end()||!it->second->texture||it->second->description.viewType!=VK_IMAGE_VIEW_TYPE_2D
        ||!(it->second->description.aspects&VK_IMAGE_ASPECT_DEPTH_BIT)
        ||!(it->second->description.usage&VK_IMAGE_USAGE_SAMPLED_BIT)) {
        error="Native comparison role requires an actual sampled 2D depth image";return false;
    }
    if(it->second->depthComparison)return true;
    auto next=std::make_shared<Record>(*it->second);next->depthComparison=true;++next->revision;
    it->second=std::move(next);return true;
}
bool QueueUpload(const GfxImage* owner,unsigned sourceFormat,unsigned face,unsigned mip,const void* source,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);const auto it=images.find(owner);
    if(it==images.end()||!source) {error="Missing native image upload owner/source";return false;}
    const auto& previous=*it->second;const auto& d=previous.description;
    if(d.aspects!=VK_IMAGE_ASPECT_COLOR_BIT) {error="Depth uploads require their native transfer provider";return false;}
    const unsigned layer=d.viewType==VK_IMAGE_VIEW_TYPE_CUBE?face:0;
    if(sourceFormat!=previous.sourceFormat||mip>=d.mipLevels||layer>=d.arrayLayers) {error="Native image upload format/subresource mismatch";return false;}
    const std::uint64_t bytes=SubresourceBytes(previous,mip);
    if(bytes>std::numeric_limits<std::size_t>::max()) {error="Native image upload overflow";return false;}
    std::shared_ptr<const UploadBytes> pixels;
    if(previous.nativeBlocks) {pixels=UploadBytes::Copy(source,std::size_t(bytes),error);if(!pixels)return false;}
    else if(previous.sourceBlockBytes) {
        const unsigned width=std::max(1u,d.extent.width>>mip),height=std::max(1u,d.extent.height>>mip),depth=std::max(1u,d.extent.depth>>mip);
        const std::uint64_t sourceBytes=((std::uint64_t(width)+3)/4)*((std::uint64_t(height)+3)/4)*depth*previous.sourceBlockBytes;
        if(sourceBytes>std::numeric_limits<std::size_t>::max()) {error="Compressed image upload overflow";return false;}
        const auto compression=sourceFormat==D3DFMT_DXT1?BlockCompression::BC1:(sourceFormat==D3DFMT_DXT3?BlockCompression::BC2:BlockCompression::BC3);
        std::vector<unsigned char> decoded;
        if(!DecodeBlocks(compression,width,height,depth,source,std::size_t(sourceBytes),decoded,error))return false;
        pixels=UploadBytes::Decoded(std::move(decoded));
    } else {pixels=UploadBytes::Copy(source,std::size_t(bytes),error);if(!pixels)return false;}
    auto next=std::make_shared<Record>(previous);++next->revision;
    auto upload=std::find_if(next->uploads.begin(),next->uploads.end(),[&](const auto& u){return u.mip==mip&&u.layer==layer;});
    if(upload==next->uploads.end())next->uploads.push_back({mip,layer,std::move(pixels)});
    else upload->bytes=std::move(pixels);
    it->second=std::move(next);image_uploads::Queue(it->second);return true;
}
bool QueueL8Plane(const GfxImage* owner,const void* source,std::size_t pitch,unsigned width,unsigned height,unsigned char clearValue,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);const auto it=images.find(owner);
    if(it==images.end()) {error="Missing native L8 upload owner";return false;}
    const auto& previous=*it->second;const auto& d=previous.description;
    if(!previous.texture||previous.sourceFormat!=D3DFMT_L8||previous.bytesPerTexel!=1||previous.sourceBlockBytes
        ||d.aspects!=VK_IMAGE_ASPECT_COLOR_BIT||d.extent.width!=width||d.extent.height!=height
        ||d.extent.depth!=1||d.mipLevels!=1||d.arrayLayers!=1) {
        error="Native L8 plane does not match its actual image";return false;
    }
    auto pixels=UploadBytes::Rows(source,width,height,pitch,clearValue,error);if(!pixels)return false;
    auto next=std::make_shared<Record>(previous);++next->revision;
    next->uploads={{0,0,std::move(pixels)}};
    it->second=std::move(next);image_uploads::Queue(it->second);return true;
}
std::shared_ptr<const Record> Acquire(const GfxImage* owner) {
    std::lock_guard<std::mutex> lock(mutex);const auto it=images.find(owner);return it==images.end()?nullptr:it->second;
}
std::string MissingOwnerDiagnostic(const GfxImage* owner) {
    const std::string name=owner&&owner->name?owner->name:"<unnamed>";
    std::string result="Native material image not registered name="+name+" owner="+std::to_string(reinterpret_cast<std::uintptr_t>(owner));
    if(owner)result+=" shape="+std::to_string(owner->width)+"x"+std::to_string(owner->height)+"x"+std::to_string(owner->depth)
        +" map="+std::to_string(owner->mapType)+" track="+std::to_string(owner->track)+" delayed="+std::to_string(owner->delayLoadPixels)+" streaming="+std::to_string(owner->streaming);
    std::lock_guard<std::mutex> lock(mutex);unsigned matched=0;
    for(const auto& image:images)if(image.second->createdName==name) {
        if(matched<4)result+=" same_name_owner="+std::to_string(reinterpret_cast<std::uintptr_t>(image.first))+" serial="+std::to_string(image.second->serial);
        ++matched;
    }
    return result+" same_name_count="+std::to_string(matched);
}
void CopyOwner(const GfxImage* source,const GfxImage* destination) {
    if(source==destination)return;
    std::lock_guard<std::mutex> lock(mutex);
    auto it=images.find(source);
    auto record=it==images.end()?std::shared_ptr<const Record>{}:it->second;
    if(record&&record->serial<=3)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","image_copy_owner serial=%llu from=%p to=%p",(unsigned long long)record->serial,source,destination);
    if(record)images[destination]=std::move(record);else images.erase(destination);
}
bool RestoreOwner(const GfxImage* owner,std::shared_ptr<const Record> record,std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);
    if(!owner||!record||!record->texture||!record->texture->Handle()||record->texture->Device()!=device||images.count(owner)) {
        error="Native image rollback requires an absent owner and retained real resource on the same device";return false;
    }
    images.emplace(owner,std::move(record));return true;
}
bool Release(const GfxImage* owner){std::lock_guard<std::mutex> lock(mutex);return images.erase(owner)!=0;}
}
