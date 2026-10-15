#pragma once
#include "gfx_vulkan/presentation.h"
#include "gfx_vulkan/graphics_program.h"
#include "gfx_vulkan/program_cache.h"
#include "gfx_vulkan/material_dispatch.h"
#include "gfx_vulkan/pass_recording.h"
#include "gfx_vulkan/occlusion_queries.h"
#include "gfx_vulkan/query_assets.h"
#include "gfx_vulkan/event_fences.h"
#include "gfx_vulkan/resources.h"
#include "gfx_vulkan/shader_assets.h"
#include "gfx_vulkan/vertex_assets.h"
#include "gfx_vulkan/image_assets.h"
#include "gfx_vulkan/texture_bindings.h"
#include "gfx_vulkan/attachment_pass.h"
#include "gfx_vulkan/render_targets.h"
#include "gfx_vulkan/buffer_assets.h"
#include "gfx_vulkan/geometry_bindings.h"
#include "gfx_vulkan/constant_state.h"
#include "gfx_vulkan/material_bindings.h"
struct XSurface;
struct GfxCmdBufSourceState;
struct IDirect3DVertexBuffer9;
struct IDirect3DQuery9;
#include "database/db_material_graph_types.h"
#include <memory>
namespace bo1::android {
// Development material integration. Uses production native resource/pipeline
// owners, but consumes a capture fixture rather than a running game scene.
class NativeMaterialPreview {
public:
    NativeMaterialPreview(bo1::vulkan::Presentation& p,const char* root);
    ~NativeMaterialPreview();
    static void Prepare(void*,const bo1::vulkan::FrameContext&);
    static void Submitted(void*,unsigned,VkFence);
    static void Draw(void*,const bo1::vulkan::FrameContext&);
    const std::string& Error()const{return error_;}
private:
    bool Prepare(const bo1::vulkan::FrameContext&);
    bool PrepareAttachments(const bo1::vulkan::FrameContext&);
    std::unique_ptr<bo1::vulkan::AttachmentPass> probePass_,scratchPass_,loadPass_,stencilPass_,depthOnlyPass_;
    std::array<std::shared_ptr<bo1::vulkan::GraphicsProgram>,7> statePrograms_;
    std::array<std::array<VkDescriptorSet,2>,7> stateSets_{};
    VkDescriptorPool statePool_{};
    std::shared_ptr<const bo1::vulkan::render_targets::Record> probeTarget_;
    std::shared_ptr<const bo1::vulkan::image_assets::Record> stencilDepth_;
    std::shared_ptr<bo1::vulkan::Texture2D> scratchDepth_;
    std::unique_ptr<bo1::vulkan::Buffer> probeReadback_;
    std::unique_ptr<bo1::vulkan::AttachmentPass> generatedSourcePass_,generatedOutputPass_;
    std::shared_ptr<bo1::vulkan::Texture2D> generatedSource_,generatedOutput_;
    std::shared_ptr<bo1::vulkan::GraphicsProgram> generatedProgram_;
    std::shared_ptr<const bo1::vulkan::texture_bindings::Snapshot> generatedBindings_;
    VkDescriptorPool generatedPool_{};
    std::shared_ptr<bo1::vulkan::Texture2D> generatedSharedDepth_;
    std::array<bo1::vulkan::pass_recording::Arena,bo1::vulkan::Presentation::FrameSlots> passFrames_;
    IDirect3DQuery9* engineFence_{};bool eventAllocated_{};
    std::array<bo1::vulkan::event_fences::Arena,bo1::vulkan::Presentation::FrameSlots> eventFrames_;
    std::unique_ptr<bo1::vulkan::event_fences::RecordingScope> eventRecording_;
    IDirect3DQuery9* engineQuery_{};bool engineQueryAllocated_{};
    std::array<bo1::vulkan::query_assets::Arena,bo1::vulkan::Presentation::FrameSlots> queryFrames_;
    std::unique_ptr<bo1::vulkan::query_assets::RecordingScope> queryRecording_;
    std::unique_ptr<bo1::vulkan::OcclusionQueries> occlusion_;bool queriesPending_{};
    bool probeChecked_{};unsigned probeSlot_{};
    bo1::vulkan::Presentation& presentation_;std::string root_,error_;
    std::shared_ptr<bo1::vulkan::GraphicsProgram> program_;
    std::unique_ptr<bo1::vulkan::ProgramCache> programCache_;
    std::array<std::vector<bo1::vulkan::material_dispatch::Packet>,bo1::vulkan::Presentation::FrameSlots> dispatchFrames_;
    std::array<VkDeviceSize,bo1::vulkan::Presentation::FrameSlots> dispatchCursors_{};
    VkDeviceSize uniformAlignment_{};bool dispatchChecked_{};
    std::shared_ptr<bo1::vulkan::Texture2D> texture_;
    std::unique_ptr<GfxImage> image_;
    std::array<std::vector<std::shared_ptr<const bo1::vulkan::geometry_bindings::Snapshot>>,bo1::vulkan::Presentation::FrameSlots> geometryFrames_;
    std::unique_ptr<XSurface> geometry_;
    std::shared_ptr<const bo1::vulkan::buffer_assets::Record> vertexBufferAsset_,indexBufferAsset_;
    std::unique_ptr<GfxCmdBufState> commandState_;
    MaterialPass materialPass_{};
    std::array<std::shared_ptr<const bo1::vulkan::material_bindings::Draw>,bo1::vulkan::Presentation::FrameSlots> drawFrames_;
    bool drawCaptureChecked_{};
    bool dynamicChecked_{};
    std::unique_ptr<GfxCmdBufSourceState> constantSource_;
    bool constantOwnershipChecked_{};
    std::array<std::shared_ptr<const bo1::vulkan::constant_state::Snapshot>,bo1::vulkan::Presentation::FrameSlots> constantFrames_;
    std::shared_ptr<const bo1::vulkan::texture_bindings::Snapshot> textureBindings_;
    std::shared_ptr<const bo1::vulkan::image_assets::Record> imageAsset_;
    std::unique_ptr<bo1::vulkan::Buffer> upload_;
    std::array<std::unique_ptr<bo1::vulkan::Buffer>,bo1::vulkan::Presentation::FrameSlots> buffers_;
    std::array<VkDescriptorPool,bo1::vulkan::Presentation::FrameSlots> pools_{};
    std::array<std::array<VkDescriptorSet,2>,bo1::vulkan::Presentation::FrameSlots> sets_{};
    VkDeviceSize pixelOffset_{},vertexOffset_{};unsigned generation_{};
    MaterialVertexShader vertexShader_{};MaterialPixelShader pixelShader_{};
    std::shared_ptr<const bo1::vulkan::shader_assets::Record> vertexAsset_,pixelAsset_;
    MaterialVertexDeclaration declaration_{};
    std::shared_ptr<const bo1::vulkan::vertex_assets::Record> declarationAsset_;
};
}
