#include "resources.h"
#include <cstring>
#include <limits>
namespace bo1::vulkan {
namespace {
bool Check(VkResult r,const char* action,std::string& e) {if(r==VK_SUCCESS)return true;e=std::string(action)+": "+std::to_string(int(r));return false;}
bool Memory(VkPhysicalDevice p,unsigned bits,VkMemoryPropertyFlags required,VkMemoryPropertyFlags preferred,unsigned& type,bool& coherent) {
    VkPhysicalDeviceMemoryProperties properties{};vkGetPhysicalDeviceMemoryProperties(p,&properties);
    int best=-1;unsigned score=0;
    for(unsigned i=0;i<properties.memoryTypeCount;++i)if((bits&(1u<<i))&&(properties.memoryTypes[i].propertyFlags&required)==required) {
        unsigned matched=properties.memoryTypes[i].propertyFlags&preferred,s=0;while(matched) {s+=matched&1u;matched>>=1;}
        if(best<0||s>score) {best=int(i);score=s;}
    }
    if(best<0)return false;type=unsigned(best);coherent=properties.memoryTypes[type].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;return true;
}
}
Buffer::~Buffer() {if(mapped_)vkUnmapMemory(device_,memory_);if(buffer_)vkDestroyBuffer(device_,buffer_,nullptr);if(memory_)vkFreeMemory(device_,memory_,nullptr);}
bool Buffer::Create(VkDeviceSize bytes,VkBufferUsageFlags usage,VkMemoryPropertyFlags required,VkMemoryPropertyFlags preferred,std::string& error) {
    if(buffer_||!bytes||bytes>std::numeric_limits<std::size_t>::max()) {error="Invalid buffer creation";return false;}
    VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};create.size=bytes;create.usage=usage;
    if(!Check(vkCreateBuffer(device_,&create,nullptr,&buffer_),"buffer",error))return false;
    VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(device_,buffer_,&requirements);
    unsigned type{};if(!Memory(physical_,requirements.memoryTypeBits,required,preferred,type,coherent_)) {error="No compatible buffer memory";return false;}
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=requirements.size;allocate.memoryTypeIndex=type;
    if(!Check(vkAllocateMemory(device_,&allocate,nullptr,&memory_),"buffer allocation",error)
        ||!Check(vkBindBufferMemory(device_,buffer_,memory_,0),"buffer binding",error))return false;
    if(required&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        if(!Check(vkMapMemory(device_,memory_,0,VK_WHOLE_SIZE,0,&mapped_),"buffer mapping",error))return false;
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical_,&properties);atomSize_=properties.limits.nonCoherentAtomSize;
    size_=bytes;allocationSize_=requirements.size;return true;
}
bool Buffer::Write(VkDeviceSize offset,const void* source,VkDeviceSize bytes,std::string& error) {
    if(!mapped_||!source||offset>size_||bytes>size_-offset) {error="Invalid mapped buffer range";return false;}
    if(!bytes)return true;std::memcpy(static_cast<char*>(mapped_)+std::size_t(offset),source,std::size_t(bytes));
    if(!coherent_) {
        VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};range.memory=memory_;range.offset=(offset/atomSize_)*atomSize_;range.size=VK_WHOLE_SIZE;
        return Check(vkFlushMappedMemoryRanges(device_,1,&range),"buffer flush",error);
    }
    return true;
}
bool Buffer::Read(VkDeviceSize offset,void* destination,VkDeviceSize bytes,std::string& error) {
    if(!mapped_||!destination||offset>size_||bytes>size_-offset) {error="Invalid mapped read range";return false;}
    if(!bytes)return true;
    if(!coherent_) {
        VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};range.memory=memory_;range.offset=(offset/atomSize_)*atomSize_;range.size=VK_WHOLE_SIZE;
        if(!Check(vkInvalidateMappedMemoryRanges(device_,1,&range),"buffer invalidate",error))return false;
    }
    std::memcpy(destination,static_cast<char*>(mapped_)+std::size_t(offset),std::size_t(bytes));return true;
}
Texture2D::~Texture2D() {if(sampler_)vkDestroySampler(device_,sampler_,nullptr);if(attachmentView_)vkDestroyImageView(device_,attachmentView_,nullptr);if(view_)vkDestroyImageView(device_,view_,nullptr);if(image_)vkDestroyImage(device_,image_,nullptr);if(memory_)vkFreeMemory(device_,memory_,nullptr);}
bool Texture2D::CreateRGBA8(unsigned width,unsigned height,std::string& error) {
    TextureDescription description;description.extent={width,height,1};
    if(!Create(description,error))return false;
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler.magFilter=sampler.minFilter=VK_FILTER_LINEAR;
    sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    return Check(vkCreateSampler(device_,&sampler,nullptr,&sampler_),"compatibility sampler",error);
}
bool Texture2D::Create(const TextureDescription& description,std::string& error) {
    const auto extent=description.extent;
    const bool cube=description.viewType==VK_IMAGE_VIEW_TYPE_CUBE;
    unsigned maxDimension=extent.width;
    if(extent.height>maxDimension)maxDimension=extent.height;
    if(extent.depth>maxDimension)maxDimension=extent.depth;
    unsigned maxMips=0;for(auto dimension=maxDimension;dimension;dimension>>=1)++maxMips;
    if(image_||!extent.width||!extent.height||!extent.depth||!description.mipLevels||description.mipLevels>maxMips
        ||(description.type!=VK_IMAGE_TYPE_2D&&description.type!=VK_IMAGE_TYPE_3D)
        ||(description.type==VK_IMAGE_TYPE_3D&&(description.viewType!=VK_IMAGE_VIEW_TYPE_3D||description.arrayLayers!=1))
        ||(description.type==VK_IMAGE_TYPE_2D&&(extent.depth!=1
            ||(!cube&&(description.viewType!=VK_IMAGE_VIEW_TYPE_2D||description.arrayLayers!=1))
            ||(cube&&(description.arrayLayers!=6||extent.width!=extent.height))))) {
        error="Invalid sampled texture description";return false;
    }
    VkFormatProperties format{};vkGetPhysicalDeviceFormatProperties(physical_,description.format,&format);
    const auto usage=description.usage;
    VkFormatFeatureFlags features=0;
    if(usage&VK_IMAGE_USAGE_SAMPLED_BIT)features|=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|(description.linearSampling?VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT:0);
    if(usage&VK_IMAGE_USAGE_TRANSFER_DST_BIT)features|=VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if(usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT)features|=VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if(usage&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)features|=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    if(usage&VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)features|=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    const VkImageUsageFlags attachmentBits=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if(!usage||!description.aspects||(usage&VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT&&
        (!(usage&attachmentBits)||(usage&~(attachmentBits|VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))))) {
        error="Invalid transient attachment/image usage";return false;
    }
    if((format.optimalTilingFeatures&features)!=features) {error="Image format lacks requested usage/filter features: required="+std::to_string(features)+" available="+std::to_string(format.optimalTilingFeatures);return false;}
    const VkImageCreateFlags flags=cube?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
    VkImageFormatProperties supported{};
    if(!Check(vkGetPhysicalDeviceImageFormatProperties(physical_,description.format,description.type,VK_IMAGE_TILING_OPTIMAL,usage,flags,&supported),"texture format capabilities",error))return false;
    if(extent.width>supported.maxExtent.width||extent.height>supported.maxExtent.height||extent.depth>supported.maxExtent.depth
        ||description.mipLevels>supported.maxMipLevels||description.arrayLayers>supported.maxArrayLayers) {
        error="Sampled texture exceeds device capabilities";return false;
    }
    VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.imageType=description.type;create.format=description.format;create.flags=flags;
    create.extent=extent;create.mipLevels=description.mipLevels;create.arrayLayers=description.arrayLayers;
    create.samples=VK_SAMPLE_COUNT_1_BIT;create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=usage;
    if(!Check(vkCreateImage(device_,&create,nullptr,&image_),"texture",error))return false;
    VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(device_,image_,&requirements);unsigned type{};bool coherent{};
    if(!Memory(physical_,requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,(usage&VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)?VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT:0,type,coherent)) {error="No texture memory";return false;}
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=requirements.size;allocate.memoryTypeIndex=type;
    if(!Check(vkAllocateMemory(device_,&allocate,nullptr,&memory_),"texture allocation",error)||!Check(vkBindImageMemory(device_,image_,memory_,0),"texture binding",error))return false;
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=image_;view.viewType=description.viewType;view.format=create.format;
    view.components=description.components;view.subresourceRange={(description.aspects&VK_IMAGE_ASPECT_DEPTH_BIT)?VK_IMAGE_ASPECT_DEPTH_BIT:description.aspects,0,description.mipLevels,0,description.arrayLayers};
    if(!Check(vkCreateImageView(device_,&view,nullptr,&view_),"texture view",error))return false;
    if(usage&(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
        view.components={};view.subresourceRange.aspectMask=description.aspects;
        if(!Check(vkCreateImageView(device_,&view,nullptr,&attachmentView_),"attachment view",error))return false;
    }
    description_=description;return true;
}
void Texture2D::Upload(VkCommandBuffer command,VkBuffer staging,VkDeviceSize offset)const {
    std::string error;UploadSubresource(command,staging,offset,0,0,VK_IMAGE_LAYOUT_UNDEFINED,error);
}
bool Texture2D::UploadSubresource(VkCommandBuffer command,VkBuffer staging,VkDeviceSize offset,unsigned mip,unsigned layer,
    VkImageLayout previousLayout,std::string& error)const {
    if(!image_||!view_||description_.aspects!=VK_IMAGE_ASPECT_COLOR_BIT||!(description_.usage&VK_IMAGE_USAGE_TRANSFER_DST_BIT)||!command||!staging||mip>=description_.mipLevels||layer>=description_.arrayLayers
        ||(previousLayout!=VK_IMAGE_LAYOUT_UNDEFINED&&previousLayout!=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
        error="Invalid sampled texture upload subresource/layout";return false;
    }
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=image_;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,mip,1,layer,1};barrier.oldLayout=previousLayout;
    barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    const bool update=previousLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask=update?VK_ACCESS_SHADER_READ_BIT:0;
    const VkPipelineStageFlags shaderStages=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    vkCmdPipelineBarrier(command,update?shaderStages:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    VkBufferImageCopy copy{};copy.bufferOffset=offset;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,mip,layer,1};
    const auto extent=description_.extent;
    copy.imageExtent={extent.width>>mip,extent.height>>mip,extent.depth>>mip};
    if(!copy.imageExtent.width)copy.imageExtent.width=1;
    if(!copy.imageExtent.height)copy.imageExtent.height=1;
    if(!copy.imageExtent.depth)copy.imageExtent.depth=1;
    vkCmdCopyBufferToImage(command,staging,image_,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,shaderStages,0,0,nullptr,0,nullptr,1,&barrier);return true;
}
}
