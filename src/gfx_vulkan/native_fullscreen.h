#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <string>
#include <unordered_map>
struct GfxImage;
namespace bo1::vulkan::native_fullscreen {
// Native full-screen effect programs (GLSL compiled to SPIR-V at build time).
// Each samples its inputs through combined image samplers at bindings
// 0..ImageCount-1 and reads one uniform block at binding 8.
enum class Program : unsigned {DofDownsample,PostCompose,BloomPrefilter,BloomRemap,BloomBlur,Count};
// r_nativeSgsr: upscale the scene to the window with Snapdragon GSR v1 when it
// is rendered below the output size (render thread reads it at present).
void SetSgsrUpscale(bool enabled);
bool SgsrUpscale();
unsigned ImageCount(Program);
struct Input {const GfxImage* image{};bool point{};};
// Render-thread owner of the program objects. Pipelines are created per
// colour-only render pass; the caller retires GPU work before destruction.
class Library {
public:
    explicit Library(VkDevice device):device_(device){}
    ~Library();
    Library(const Library&)=delete;Library& operator=(const Library&)=delete;
    bool Initialize(std::string& error);
    bool Acquire(Program,VkRenderPass,VkPipeline&,VkPipelineLayout&,VkDescriptorSetLayout&,std::string& error);
    VkSampler Sampler(bool point)const{return point?point_:linear_;}
private:
    struct Entry {VkShaderModule fragment{};VkDescriptorSetLayout set{};VkPipelineLayout layout{};std::unordered_map<VkRenderPass,VkPipeline> pipelines;};
    VkDevice device_{};VkShaderModule vertex_{};VkSampler linear_{},point_{};
    std::array<Entry,unsigned(Program::Count)> programs_{};
};
}
