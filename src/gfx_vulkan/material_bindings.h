#pragma once
#include "shader_assets.h"
#include "vertex_assets.h"
#include "constant_state.h"
#include "texture_bindings.h"
#include "geometry_bindings.h"
#include "draw_storage.h"
struct GfxCmdBufState;
namespace bo1::vulkan {struct BindCache;struct ResourceScratch;}
namespace bo1::vulkan::material_bindings {
struct Selection {std::shared_ptr<const shader_assets::Record> vertex,pixel;std::shared_ptr<const vertex_assets::Record> declaration;unsigned variant{};};
struct Draw {std::shared_ptr<const Selection> selection;std::shared_ptr<const constant_state::Snapshot> constants;
    std::shared_ptr<const texture_bindings::Snapshot> textures;std::shared_ptr<const geometry_bindings::Snapshot> geometry;unsigned words[2]{};
    float polygonBias{},polygonSlope{};};
bool Vertex(const GfxCmdBufState*,const MaterialVertexShader*,std::string&);
bool Pixel(const GfxCmdBufState*,const MaterialPixelShader*,std::string&);
bool Declaration(const GfxCmdBufState*,const MaterialVertexDeclaration*,unsigned,std::string&);
// Remove selected stages/declaration; captured draws retain their snapshots.
// mask: vertex=1, pixel=2, declaration=4.
bool Clear(const GfxCmdBufState*,unsigned mask,std::string&);
std::shared_ptr<const Selection> Acquire(const GfxCmdBufState*);
bool Capture(const GfxCmdBufState*,Draw&,std::string&);
std::shared_ptr<const Draw> Capture(const GfxCmdBufState*,std::string&);
void Release(const GfxCmdBufState*);void Clone(const GfxCmdBufState*,const GfxCmdBufState*);
// Fresh ranges for each draw; reset cursor only after the arena's fence retires.
// The caller owns/reserves the buffer and descriptor pool for submitted readers.
bool Upload(const Draw&,Buffer&,VkDeviceSize& cursor,VkDeviceSize alignment,
    std::vector<ProgramResource>& vs,std::vector<ProgramResource>& ps,std::string&);
bool Upload(const Draw&,DrawStorage&,VkDeviceSize alignment,
    std::vector<ProgramResource>& vs,std::vector<ProgramResource>& ps,VkDescriptorPool&,std::string&,const unsigned* constantRows=nullptr,BindCache* cache=nullptr);
// Internal native path: outputs belong exclusively to a live scratch lease.
// Failure may leave scratch contents, never caller-owned public Upload outputs.
bool UploadScratch(const Draw&,DrawStorage&,VkDeviceSize,ResourceScratch&,VkDescriptorPool&,std::string&,
    const unsigned* constantRows=nullptr,BindCache* cache=nullptr);
}
