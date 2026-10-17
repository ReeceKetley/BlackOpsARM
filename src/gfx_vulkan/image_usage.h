#pragma once
#include "resources.h"
#include "pointer_hash.h"
#include <memory>
#include <unordered_map>
namespace bo1::vulkan::image_usage {
struct State {std::weak_ptr<Texture2D> image;VkImageLayout layout=VK_IMAGE_LAYOUT_UNDEFINED;bool contents=false,active=false;};
using States=std::unordered_map<const Texture2D*,State,bo1::vulkan::PointerHash>;
// One queue/render-thread owner. Published layouts describe ordered successful
// submissions, not completed GPU work. Resource borrowers still need fences.
struct Tracker {States submitted;};
class RecordingScope {
public:
    RecordingScope(Tracker&,VkCommandBuffer);~RecordingScope();
    RecordingScope(const RecordingScope&)=delete;RecordingScope& operator=(const RecordingScope&)=delete;
    bool Validate(std::string&)const;void Commit();
    State Lookup(const std::shared_ptr<Texture2D>&)const;
    Tracker& tracker;VkCommandBuffer command;States pending;
private:RecordingScope* previous_;
};
// Parallel scene recording: a worker thread samples against the recording
// thread's scope while that thread records no transitions/copies/passes (the
// fork-join brackets one already-open pass). Begin/End/Copy still require the
// owner's own command buffer, so a worker cannot change tracked layouts.
const RecordingScope* CurrentScope();
class BorrowScope {
public:
    explicit BorrowScope(const RecordingScope*);~BorrowScope();
    BorrowScope(const BorrowScope&)=delete;BorrowScope& operator=(const BorrowScope&)=delete;
private:RecordingScope* previous_;
};
bool Begin(VkCommandBuffer,const std::shared_ptr<Texture2D>& color,const std::shared_ptr<Texture2D>& depth,bool loadColor,bool loadDepth,std::string&);
bool End(VkCommandBuffer,const std::shared_ptr<Texture2D>& color,const std::shared_ptr<Texture2D>& depth,bool preserveDepth,std::string&);
bool CanLoad(const std::shared_ptr<Texture2D>&);
bool Known(const std::shared_ptr<Texture2D>&);
// Record a same-format, whole 2D generated-colour/depth image copy (all aspects). Caller retains both images.
// copyAspects limits which aspects carry contents (0 = all). Layout ownership
// still covers the whole image; uncopied destination aspects are undefined.
bool Copy(VkCommandBuffer,const std::shared_ptr<Texture2D>& source,const std::shared_ptr<Texture2D>& destination,std::string&,VkImageAspectFlags copyAspects=0);
// Rectangle copy of the named aspects between same-format single-mip 2D images.
// Destination texels outside the rectangle keep their contents only when the
// destination already holds preserved contents (from UNDEFINED they are undefined).
bool CopyRegion(VkCommandBuffer,const std::shared_ptr<Texture2D>& source,VkOffset2D sourceOffset,const std::shared_ptr<Texture2D>& destination,
    VkOffset2D destinationOffset,VkExtent2D extent,VkImageAspectFlags copyAspects,std::string&);
// Resolves tracking once. Unknown uploaded images retain their existing upload path.
bool ResolveSampling(const std::shared_ptr<Texture2D>&,bool requireTracked,VkImageLayout&,bool& known,std::string&);
std::uint64_t SamplingLookupCount();
bool Sample(const std::shared_ptr<Texture2D>&,VkImageLayout&,std::string&);
}
