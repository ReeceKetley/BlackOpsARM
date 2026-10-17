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
    // Pixel bytes of already uploaded subresources were released after their
    // staging copy (the GPU image holds them); such uploads have null bytes.
    bool uploadsReleased{};
    // Exact tile differences from regionBaseRevision; full bytes remain recovery data.
    bool modelLightingRegions{};std::uint64_t regionBaseRevision{};
    std::vector<unsigned> changedTiles;
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
// After a flush copied these records' bytes into staging: replace each one
// that is still current with a copy holding no pixel bytes (CPU memory is
// freed once older snapshots retire). Returns (old, new) replacements.
std::vector<std::pair<std::shared_ptr<const Record>,std::shared_ptr<const Record>>> ReleaseUploadedBytes(
    const std::vector<std::shared_ptr<const Record>>& uploaded);
void SetModelLightingRegions(bool);
bool ModelLightingRegionsEnabled();
void SetModelLightingBoxes(bool);
bool ModelLightingBoxesEnabled();
struct ModelLightingProfile {std::uint64_t batches{},changedTiles{},unchangedTiles{},snapshotBytes{},cpuNs{};};
void SetModelLightingProfileTiming(bool);
ModelLightingProfile ReadModelLightingProfile();
bool QueueModelLighting(const GfxImage*,const void*,const std::vector<unsigned>& tiles,std::string&);
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
