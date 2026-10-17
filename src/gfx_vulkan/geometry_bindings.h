#pragma once
#include "buffer_assets.h"
#include "bind_cache.h"
#include <array>
#include <vector>
struct GfxCmdBufPrimState;
namespace bo1::vulkan::geometry_bindings {
struct Stream {std::shared_ptr<const buffer_assets::Record> buffer;VkDeviceSize offset{};unsigned stride{};};
struct Snapshot {std::array<Stream,3> streams;std::shared_ptr<const buffer_assets::Record> indices;};
struct StreamSource {const void* owner{};unsigned offset{},stride{};};
// Validate every stream before atomically publishing one immutable snapshot.
bool SetStreams(const GfxCmdBufPrimState*,const std::array<StreamSource,3>&,std::string&);
bool SetVertex(const GfxCmdBufPrimState*,const void* owner,unsigned offset,unsigned stride,std::string&);
void SetIndexBindingReuse(bool enabled);
bool IndexBindingReuseEnabled();
struct IndexBindingProfile {std::uint64_t attempts{},hits{};};
IndexBindingProfile ReadIndexBindingProfile();
bool SetIndex(const GfxCmdBufPrimState*,const void* owner,std::string&);
// Clear vertex streams without dropping indices or immutable captured draws.
bool ClearVertices(const GfxCmdBufPrimState*,std::string&);
void Clone(const GfxCmdBufPrimState*,const GfxCmdBufPrimState*);
void Release(const GfxCmdBufPrimState*);
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufPrimState*);
// Caller has bound the material/pipeline and begun a compatible render pass.
// The frame arena is cleared only after its submission fence retires.
class RecordingScope {
public:
    RecordingScope(const GfxCmdBufPrimState*,VkCommandBuffer,std::vector<std::shared_ptr<const Snapshot>>&,std::shared_ptr<const Snapshot> captured=nullptr,BindCache* binds=nullptr);
    ~RecordingScope();RecordingScope(const RecordingScope&)=delete;RecordingScope& operator=(const RecordingScope&)=delete;
    std::shared_ptr<const Snapshot> captured;
    const GfxCmdBufPrimState* state;VkCommandBuffer command;std::vector<std::shared_ptr<const Snapshot>>& arena;BindCache* binds;
private:RecordingScope* previous_;
};
// mergeable: the caller proved this draw's pipeline/descriptor state equals
// the pending draw's, so a contiguous index range may extend it.
bool DrawIndexed(const GfxCmdBufPrimState*,int baseIndex,int vertexCount,int triCount,std::string&,bool mergeable=false);
// Explicit native path: frameRetained is already held by a fence-retired
// packet, with the same GPU bindings as captured. No transient arena append.
// This does not transfer retention responsibility away from the caller.
bool DrawIndexedRetained(VkCommandBuffer,const std::shared_ptr<const Snapshot>& captured,
    const std::shared_ptr<const Snapshot>& frameRetained,BindCache*,int baseIndex,int vertexCount,int triCount,std::string&,bool mergeable=false);
struct RecordingProfile {std::uint64_t arenaRetentions{},arenaGrowths{},retainedDraws{};};
void ResetRecordingProfile();RecordingProfile ReadRecordingProfile();
// Same GPU vertex/index buffers, offsets and strides would be bound.
bool SameBindings(const Snapshot&,const Snapshot&);
}
