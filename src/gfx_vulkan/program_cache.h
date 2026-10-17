#pragma once
#include "pipeline_key.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <string>
namespace bo1::vulkan {
// Render-thread owner; Acquire may also be called by parallel scene recording
// threads (all entry points serialize on one lock). Each submitted frame must retain the returned
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
    void Clear(){std::lock_guard<std::recursive_mutex> lock(mutex_);programs_.clear();shaderPairs_.clear();recipeBuilt_.assign(recipes_.size(),false);}
    // Program recipes: every program created with registered shaders and a
    // known render-pass key is recorded (saved beside the driver cache) so a
    // later session can build it while a map loads instead of mid-game.
    struct PrewarmResult {unsigned built{},pending{},failed{};double ms{};};
    PrewarmResult Prewarm(const std::function<VkRenderPass(const std::array<unsigned,8>&)>& pass);
    std::size_t Recipes()const{return recipes_.size();}
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
    std::recursive_mutex mutex_; // Prewarm re-enters Acquire
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
    void RecordRecipe(const ProgramDescription&);
    bool SaveRecipes(std::string&)const;void LoadRecipes();
    std::vector<std::vector<unsigned char>> recipes_;std::vector<bool> recipeBuilt_;
    std::unordered_set<std::uint64_t> recipeHashes_;std::size_t savedRecipes_{};
    std::unordered_map<const shader_assets::Record*,std::uint64_t> shaderHashes_;bool prewarming_{};
};
}
