#pragma once
#include <vulkan/vulkan.h>
#include <cstddef>
#include <string>
namespace bo1::vulkan {
// GPU users must be retired before releasing these owners. Buffers may be
// persistently mapped; higher-level frame/ring ownership prevents overwrite.
class Buffer {
public:
    Buffer(VkPhysicalDevice physical,VkDevice device):physical_(physical),device_(device){}
    ~Buffer();Buffer(const Buffer&)=delete;Buffer& operator=(const Buffer&)=delete;
    bool Create(VkDeviceSize,VkBufferUsageFlags,VkMemoryPropertyFlags required,VkMemoryPropertyFlags preferred,std::string&);
    bool Write(VkDeviceSize offset,const void*,VkDeviceSize bytes,std::string&);
    bool Read(VkDeviceSize offset,void*,VkDeviceSize bytes,std::string&);
    VkBuffer Handle()const{return buffer_;}VkDeviceSize Size()const{return size_;}
    // Initialization owner only; stop writing before publishing/submitting.
    void* MappedForInitialization()const{return mapped_;}
private:
    VkPhysicalDevice physical_{};VkDevice device_{};VkBuffer buffer_{};VkDeviceMemory memory_{};
    VkDeviceSize size_{},allocationSize_{},atomSize_{};void* mapped_{};bool coherent_{};
};
struct TextureDescription {
    VkImageType type=VK_IMAGE_TYPE_2D;
    VkImageViewType viewType=VK_IMAGE_VIEW_TYPE_2D;
    VkFormat format=VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent3D extent{1,1,1};
    unsigned mipLevels=1,arrayLayers=1;
    VkComponentMapping components{};
    VkImageUsageFlags usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    VkImageAspectFlags aspects=VK_IMAGE_ASPECT_COLOR_BIT;
    bool linearSampling=true;
};
class Texture2D {
public:
    Texture2D(VkPhysicalDevice physical,VkDevice device):physical_(physical),device_(device){}
    ~Texture2D();Texture2D(const Texture2D&)=delete;Texture2D& operator=(const Texture2D&)=delete;
    bool CreateRGBA8(unsigned width,unsigned height,std::string&);
    bool Create(const TextureDescription&,std::string&);
    // Record outside a render pass. Staging must survive submission retirement.
    void Upload(VkCommandBuffer,VkBuffer staging,VkDeviceSize offset)const;
    // Caller tracks submitted layout per subresource and retires readers before
    // updates. Recording alone does not publish a new layout or ready texture.
    bool UploadSubresource(VkCommandBuffer,VkBuffer,VkDeviceSize,unsigned mip,unsigned layer,
        VkImageLayout previousLayout,std::string&)const;
    VkDevice Device()const{return device_;}
    VkImage Handle()const{return image_;}
    VkImageView AttachmentView()const{return attachmentView_;}
    const TextureDescription& Description()const{return description_;}
    // Render-thread record of whether stencil contents ever left their initial
    // cleared-to-zero state (stencil-enabled draw or non-zero stencil clear).
    void MarkStencilTouched()const{stencilTouched_=true;}
    bool StencilTouched()const{return stencilTouched_;}
    VkImageView View()const{return view_;}VkSampler Sampler()const{return sampler_;}
private:
    VkPhysicalDevice physical_{};VkDevice device_{};VkImage image_{};VkDeviceMemory memory_{};
    VkImageView view_{},attachmentView_{};VkSampler sampler_{};TextureDescription description_{};
    mutable bool stencilTouched_{};
};
}
