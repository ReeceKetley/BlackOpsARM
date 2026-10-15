#pragma once
#include "pipeline_key.h"
#include <memory>
#include <unordered_map>
#include <string>
namespace bo1::vulkan {
// Single render-thread owner. Each submitted frame must retain the returned
// program until its fence completes. Clear at a drained pass/device boundary;
// never reuse keys across a destroyed/recreated render pass or device.
class ProgramCache {
public:
    explicit ProgramCache(VkDevice device,unsigned capacity=64):device_(device),capacity_(capacity){}
    ~ProgramCache();
    ProgramCache(const ProgramCache&)=delete;
    ProgramCache& operator=(const ProgramCache&)=delete;
    // path/properties enable a persisted driver cache. Stored data is used
    // only when its header matches this exact device and pipeline-cache UUID.
    bool Initialize(std::string& error,std::string path={},const VkPhysicalDeviceProperties* properties=nullptr);
    // Writes the driver cache atomically (temp file + rename) when non-empty.
    bool Save(std::string& error);
    std::shared_ptr<GraphicsProgram> Acquire(const ProgramDescription&,std::string& error);
    void EraseRenderPass(VkRenderPass);
    void Clear(){programs_.clear();shaderPairs_.clear();}
    // Key timing reads the clock per draw; enable only on reported frames.
    void SetTiming(bool enabled,unsigned frame=0){timing_=enabled;frame_=frame;}
    unsigned Hits()const{return hits_;}unsigned Misses()const{return misses_;}
    std::uint64_t KeyNanoseconds()const{return keyNs_;}
    std::uint64_t CreateNanoseconds()const{return createNs_;}
    unsigned Evictions()const{return evictions_;}
    std::size_t Size()const{return programs_.size();}
    std::size_t ShaderPairs()const{return shaderPairs_.size();}
    unsigned ShaderPairHits()const{return shaderPairHits_;}
private:
    VkDevice device_{};VkPipelineCache driver_{};unsigned capacity_{},hits_{},misses_{};std::string path_;bool timing_{};
    std::uint64_t keyNs_{},createNs_{};unsigned evictions_{},frame_{};
    std::unordered_map<PipelineKey,std::shared_ptr<GraphicsProgram>,PipelineKeyHash> programs_;
    struct ShaderPair {
        // Retain immutable bytecode owners: addresses cannot alias after unload.
        std::shared_ptr<const shader_assets::Record> vertex,pixel;
        unsigned alphaComparison{},alphaReference{},depthMask{},ubyteMask{};bool relaxed{};
        std::array<ShaderTranslation,2> shaders;
    };
    std::vector<ShaderPair> shaderPairs_;unsigned shaderPairHits_{};
};
}
