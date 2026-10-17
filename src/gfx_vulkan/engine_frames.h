#pragma once
#include "presentation.h"
#include "pass_recording.h"
#include "query_assets.h"
#include "event_fences.h"
#include "frame_cadence.h"
namespace bo1::vulkan {
// Engine-owned recording storage. Bounded allocations fail explicitly; each
// slot keeps descriptors, resources and programs until its actual GPU fence.
class EngineFrames {
public:
    explicit EngineFrames(Presentation&);
    ~EngineFrames();
    bool Initialize(std::string&);
    FrameResult Begin(GfxCmdBufState*,std::string&);
    bool Submit(bool wait,std::string&);
    FrameResult Present(std::shared_ptr<const image_assets::Record>,std::string&);
    bool Drain(std::string&);
    bool Active()const{return bool(passes_);}
    const FrameContext& Context()const{return context_;}
private:
    static void Submitted(void*,unsigned,VkFence);
    Presentation& renderer_;ProgramCache cache_;AttachmentPassCache passCache_;native_fullscreen::Library fullscreen_;
    struct Slot {
        struct Profile {
            bool pending{};unsigned frame{};
            double begin{},upload{},slotWait{},retire{},reset{},prepare{},record{},submit{},wait{},submitClock{},key{},create{};
            unsigned hits{},misses{},evictions{};std::size_t cacheSize{},definitions{};
            material_dispatch::Profile draw;
        } profile;
        VkQueryPool timestamps{};
        std::unique_ptr<Buffer> uniforms;std::unique_ptr<DrawStorage> storage;VkDescriptorPool descriptors{};
        VkDeviceSize cursor{};std::vector<material_dispatch::Packet> packets;
        std::shared_ptr<GraphicsProgram> windowProgram;
        std::shared_ptr<const image_assets::Record> windowImage;
        pass_recording::Arena passes;query_assets::Arena queries;event_fences::Arena events;
        // Parallel scene recording lanes (index 0 = render thread).
        std::array<pass_recording::ParallelLane,pass_recording::MaxParallelLanes> lanes;
    };
    // deferWorkers: lanes 1.. are only marked; their recording thread retires them.
    bool RetireLanes(Slot&,std::string&,bool deferWorkers=false);
    bool RetireProfile(Slot&,std::string&);
    // Snapdragon GSR v1 window upscale (native_post/sgsr_present.*), built for
    // the current window render pass.
    struct Sgsr {VkShaderModule vertex{},fragment{};VkDescriptorSetLayout set{};VkPipelineLayout layout{};VkPipeline pipeline{};VkRenderPass pass{};} sgsr_;
    bool SgsrPipeline(std::string&);
    // Recorded programs are rebuilt after each program-cache clear (map
    // transitions) and again when new shaders register (map load completes).
    bool prewarmPending_=true;std::uint64_t prewarmShaderGeneration_{};unsigned prewarmFrame_{},prewarmClearFrame_{};
    void Prewarm();
    std::array<Slot,Presentation::FrameSlots> slots_;FrameContext context_{};VkDeviceSize alignment_{};
    std::unique_ptr<pass_recording::RecordingScope> passes_;
    std::unique_ptr<query_assets::RecordingScope> queries_;
    std::unique_ptr<event_fences::RecordingScope> events_;
    std::string submitError_;unsigned windowGeneration_{};
    std::array<std::unique_ptr<texture_bindings::Sampler>,2> outputSamplers_;
    FrameCadence cadence_;
    double timestampPeriod_{};unsigned timestampBits_{},profileFrame_{};
    std::uint64_t recordStart_{},keyStart_{},createStart_{},lastSubmit_{};
    unsigned hitStart_{},missStart_{},evictStart_{},savedMisses_{},quietFrames_{},lastMisses_{};double uploadMs_{},beginMs_{},slotWaitMs_{},retireMs_{},resetMs_{},prepareMs_{};
};
}
