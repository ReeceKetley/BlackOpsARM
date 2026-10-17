#pragma once
#include "image_assets.h"
#include "graphics_program.h"
#include <array>
struct GfxCmdBufState;
namespace bo1::vulkan::texture_bindings {
class Sampler {
public:
    explicit Sampler(VkDevice device):device_(device){}
    ~Sampler();
    // Weak cache never extends device-object lifetime beyond real users.
    static std::shared_ptr<Sampler> Acquire(VkDevice,unsigned decoded,unsigned mips,float bias,std::string&,bool comparison=false);
    static bool Validate(unsigned decoded,float bias,std::string& error);
    Sampler(const Sampler&)=delete;Sampler& operator=(const Sampler&)=delete;
    bool Create(unsigned decoded,unsigned mips,float bias,std::string&,bool comparison=false);
    VkSampler Handle()const{return sampler_;}
    bool Comparison()const{return comparison_;}
private:VkDevice device_{};VkSampler sampler_{};bool comparison_{};
};
struct Binding {
    std::shared_ptr<const image_assets::Record> image;
    std::shared_ptr<Sampler> sampler;
    unsigned decoded{};float bias{};
};
struct Snapshot {std::array<Binding,16> slots;};
bool Set(const GfxCmdBufState*,unsigned,const GfxImage*,unsigned decoded,float bias,std::string&);
bool Update(const GfxCmdBufState*,unsigned,unsigned decoded,float bias,std::string&);
void Disable(const GfxCmdBufState*,unsigned);
void Release(const GfxCmdBufState*);
void Clone(const GfxCmdBufState* source,const GfxCmdBufState* destination);
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufState*);
// Caller records uploads/barriers before drawing and retains this snapshot
// alongside its descriptor sets until the submission fence retires.
void SetSamplingValidationReuse(bool);
bool SamplingValidationReuseEnabled();
struct SamplingValidationProfile {std::uint64_t lookups{},slotReuses{},slotValidations{};};
SamplingValidationProfile ReadSamplingValidationProfile();
// Scratch must be distinct from resources; destination is unchanged on failure.
bool AppendResourcesWithScratch(const Snapshot&,const ShaderTranslation&,std::vector<ProgramResource>&,
    std::vector<ProgramResource>& scratch,std::string&);
bool AppendResources(const Snapshot&,const ShaderTranslation&,std::vector<ProgramResource>&,std::string&);
}
