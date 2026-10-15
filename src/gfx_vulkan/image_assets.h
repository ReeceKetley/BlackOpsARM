#pragma once
#include "resources.h"
#include "upload_bytes.h"
#include <memory>
#include <cstdint>
#include <vector>
struct GfxImage;
namespace bo1::vulkan::image_assets {
struct Upload {
    unsigned mip{},layer{};
    std::shared_ptr<const UploadBytes> bytes;
};
struct Record {
    std::uint64_t serial{},revision{};
    unsigned sourceFormat{},bytesPerTexel{},sourceBlockBytes{};
    std::string createdName;
    bool depthComparison{};
    bool nativeBlocks{}; // original DXT blocks uploaded to a BC format
    TextureDescription description;
    std::shared_ptr<Texture2D> texture;
    std::vector<Upload> uploads;
};
// Attach/detach at a drained renderer boundary. A device cannot be replaced
// while registry owners OR borrowed GPU owners survive.
// blockCompression: the device enabled textureCompressionBC, so DXT sources
// keep their original blocks instead of being decoded to RGBA8.
bool Attach(VkPhysicalDevice,VkDevice,std::string&,bool blockCompression=false);
// Upload byte size of one subresource (texel or 4x4-block packed).
std::uint64_t SubresourceBytes(const Record&,unsigned mip);
bool Empty();
bool Create(GfxImage*,unsigned mipCount,int flags,unsigned sourceFormat,std::string&);
// Mark only actual shadow depth images, not scene-depth postprocess inputs.
bool EnableDepthComparison(const GfxImage*,std::string&);
bool QueueUpload(const GfxImage*,unsigned sourceFormat,unsigned face,unsigned mip,const void*,std::string&);
bool QueueL8Plane(const GfxImage*,const void* source,std::size_t pitch,unsigned width,unsigned height,unsigned char clearValue,std::string&);
std::shared_ptr<const Record> Acquire(const GfxImage*);
// Failure-only provenance; does not change resources or retry missing owners.
std::string MissingOwnerDiagnostic(const GfxImage*);
// Roll back a failed replacement using an actual previously acquired record.
bool RestoreOwner(const GfxImage*,std::shared_ptr<const Record>,std::string&);
bool Release(const GfxImage*);
// Mirror a canonical DB struct copy; immutable resource snapshots retain old
// borrowers independently. No COM pointer or readiness is synthesized.
void CopyOwner(const GfxImage* source,const GfxImage* destination);
}
