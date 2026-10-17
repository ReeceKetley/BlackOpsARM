#include "material_dispatch.h"
#include "material_state.h"
#include "shader_dump.h"
#include "geometry_packet.h"
#include <chrono>
#include <atomic>
#include <cstring>
#include "gfx_d3d/rb_state.h"
namespace bo1::vulkan::material_dispatch {
namespace {thread_local RecordingScope* current{};thread_local Profile profile;thread_local bool timing{};std::atomic<bool> cachedGather{true},constantSnapshotReuse{true},preparedDrawReuse{true},geometryRetentionReuse{},geometryPacketReuse{};
// Per-draw phase timing reads the clock eight times per draw; only frames whose
// profile is actually reported pay for it.
std::uint64_t Now(){return timing?std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count():0;}
void Phase(std::uint64_t& field,std::uint64_t& start){if(!timing)return;const auto now=Now();field+=now-start;start=now;}
}
void ResetProfile(bool timed){profile={};timing=timed;geometry_bindings::ResetRecordingProfile();}
bool ProfileTimed(){return timing;}
void MergeProfile(const Profile& p){
    profile.capture+=p.capture;profile.resolve+=p.resolve;profile.state+=p.state;profile.program+=p.program;
    profile.upload+=p.upload;profile.descriptors+=p.descriptors;profile.geometry+=p.geometry;
    profile.draws+=p.draws;profile.reused+=p.reused;profile.missState+=p.missState;profile.missSelection+=p.missSelection;
    profile.missConstants+=p.missConstants;profile.missTextures+=p.missTextures;profile.missGeometry+=p.missGeometry;
    profile.cachedGroups+=p.cachedGroups;profile.cachedModels+=p.cachedModels;profile.cachedIndices+=p.cachedIndices;profile.cachedUploads+=p.cachedUploads;
    profile.programReuse+=p.programReuse;profile.fullPrepares+=p.fullPrepares;profile.weakConstants+=p.weakConstants;profile.strongConstants+=p.strongConstants;
    profile.forwardDraws+=p.forwardDraws;profile.geometryOffsetMisses+=p.geometryOffsetMisses;profile.geometryPacketHits+=p.geometryPacketHits;
    profile.geometryBufferMisses+=p.geometryBufferMisses;
}
Profile ReadProfile(){auto result=profile;const auto geometry=geometry_bindings::ReadRecordingProfile();
    result.geometryArenaRetentions=geometry.arenaRetentions;result.geometryArenaGrowths=geometry.arenaGrowths;result.geometryRetainedDraws=geometry.retainedDraws;return result;}
namespace {std::atomic<bool> resourceScratchReuse{false};}
void SetResourceScratchReuse(bool value){resourceScratchReuse.store(value,std::memory_order_relaxed);}
bool ResourceScratchReuseEnabled(){return resourceScratchReuse.load(std::memory_order_relaxed);}
ResourceScratchProfile ReadResourceScratchProfile(){return resourceScratchProfile;}
void SetGeometryPacketReuse(bool enabled){geometryPacketReuse.store(enabled,std::memory_order_relaxed);}
bool GeometryPacketReuseEnabled(){return geometryPacketReuse.load(std::memory_order_relaxed);}
void SetGeometryRetentionReuse(bool enabled){geometryRetentionReuse.store(enabled,std::memory_order_relaxed);}
bool GeometryRetentionReuseEnabled(){return geometryRetentionReuse.load(std::memory_order_relaxed);}
void SetCachedGather(bool enabled){cachedGather.store(enabled,std::memory_order_relaxed);}
bool CachedGatherEnabled(){return cachedGather.load(std::memory_order_relaxed);}
void SetConstantSnapshotReuse(bool enabled){constantSnapshotReuse.store(enabled,std::memory_order_relaxed);}
bool ConstantSnapshotReuseEnabled(){return constantSnapshotReuse.load(std::memory_order_relaxed);}
void SetPreparedDrawReuse(bool enabled){preparedDrawReuse.store(enabled,std::memory_order_relaxed);}
bool PreparedDrawReuseEnabled(){return preparedDrawReuse.load(std::memory_order_relaxed);}
void RecordCachedGroup(unsigned models,unsigned indices,unsigned uploads){++profile.cachedGroups;profile.cachedModels+=models;profile.cachedIndices+=indices;profile.cachedUploads+=uploads;}
RecordingScope::RecordingScope(const GfxCmdBufState* s,VkCommandBuffer c,VkPhysicalDevice p,Pass target,
    ProgramCache& pc,Buffer& u,VkDescriptorPool dp,VkDeviceSize& offset,VkDeviceSize align,std::vector<Packet>& retained,DrawStorage* pages,BindCache* bindCache,const unsigned* words)
    :state(s),command(c),physical(p),pass(target),cache(pc),uniforms(u),pool(dp),cursor(offset),alignment(align),packets(retained),storage(pages),binds(bindCache),previous_(current){
    if(words){effectiveWords={words[0],words[1]};overrideState=true;}current=this;
}
RecordingScope::~RecordingScope(){current=previous_;}
bool HasScope(){return current!=nullptr;}
bool DrawIndexed(const GfxCmdBufPrimState* state,int baseIndex,int vertexCount,int triCount,std::string& error) {
    if(!current||!current->state||&current->state->prim!=state||!current->command||!current->physical||!current->pass.handle||!current->pool) {
        error="Native material draw has no matching pass/frame recording scope";return false;
    }
    auto& scope=*current;auto timing=Now();
    material_bindings::Draw captured;auto* draw=&captured;
    if(!material_bindings::Capture(scope.state,captured,error))return false;
    // The pass owner used these same effective words for depth-generation
    // tracking. Capture/assets retain canonical state; only this draw changes.
    if(scope.overrideState) {
        captured.words[0]=scope.effectiveWords[0];captured.words[1]=scope.effectiveWords[1];
        ++profile.forwardDraws;
    }
    const auto& selected=*draw->selection;
    PreparedProgramStamp stamp;stamp.cache=&scope.cache;stamp.physical=scope.physical;
    stamp.pass=scope.pass.handle;stamp.pipelinePass=scope.pass.pipelineHandle?scope.pass.pipelineHandle:scope.pass.handle;
    stamp.subpass=scope.pass.subpass;stamp.samples=scope.pass.samples;stamp.color=scope.pass.color;
    stamp.depth=scope.pass.depth;stamp.stencil=scope.pass.stencil;stamp.depthFormat=scope.pass.depthFormat;
    stamp.relaxed=RelaxedPrecisionEnabled();stamp.depthOnly=!scope.pass.color&&DepthOnlyPipelines();stamp.vertex=selected.vertex.get();stamp.pixel=selected.pixel.get();
    stamp.declaration=selected.declaration.get();stamp.variant=selected.variant;
    stamp.words={draw->words[0],draw->words[1]};stamp.bias=draw->polygonBias;stamp.slope=draw->polygonSlope;
    for(unsigned i=0;i<draw->geometry->streams.size();++i)if(draw->geometry->streams[i].buffer) {
        stamp.streamMask|=1u<<i;stamp.strides[i]=draw->geometry->streams[i].stride;
    }
    // Validate specialization from the same captured textures used by Upload.
    for(const auto& binding:selected.pixel->translation.bindings) {
        if(binding.kind!=ResourceKind::Sampler)continue;
        if(binding.registerIndex>=16||!draw->textures) {error="Native sampler capture missing";return false;}
        const auto& sampled=draw->textures->slots[binding.registerIndex];
        if(sampled.image&&sampled.image->depthComparison) {
            if(!sampled.sampler||!sampled.sampler->Comparison()) {error="Native shadow image lacks its comparison sampler";return false;}
            stamp.depthSamplerMask|=1u<<binding.registerIndex;
        }
    }
    if(scope.binds&&scope.binds->last.valid) {
        // D3D9 runtimes absorbed redundant re-issued state; do the same here:
        // a draw identical to the previous one except for its index range
        // reuses the already bound pipeline, descriptor sets and buffers.
        auto& last=scope.binds->last;
        if(!(last.state==scope.state&&last.pass==scope.pass.handle
            &&last.programStamp==stamp))++profile.missState;
        else {
            const auto& selection=draw->selection;const auto& constants=draw->constants;
            const auto& textures=draw->textures;const auto& geometry=draw->geometry;
            // Geometry compares by the GPU buffers actually bound: appends to the
            // dynamic index buffer publish new records over the same buffer.
            const auto* lastGeometry=static_cast<const geometry_bindings::Snapshot*>(last.geometry.get());
            // Setting shaders publishes a new selection object even when the
            // shaders are unchanged: compare what it selects, not its identity.
            const auto* lastSelection=static_cast<const material_bindings::Selection*>(last.selection.get());
            const bool sameSelection=selection&&lastSelection&&(selection==last.selection
                ||(selection->vertex==lastSelection->vertex&&selection->pixel==lastSelection->pixel
                    &&selection->declaration==lastSelection->declaration&&selection->variant==lastSelection->variant));
            // A program that samples no textures (e.g. depth-only shadow casters)
            // is unaffected by the texture slots the material happens to bind.
            const bool sameTextures=!last.samplesTextures||textures==last.textures;
            // The bound uniforms hold the previous draw's constants. They remain
            // correct when every row the bound program reads is unchanged, even
            // if the material rewrote rows this program never reads.
            // Constants matter only when the selection and textures match (both
            // reuse paths need all three): skip the row comparisons otherwise.
            bool sameConstants=constants&&constants==last.constants&&constants->revision==last.revision;
            const bool compareConstants=sameSelection&&sameTextures;
            if(compareConstants&&constants&&last.constantStorage) {
                const void* data[2]={constants->floats[0].data(),constants->floats[1].data()};
                sameConstants=scope.binds->UploadedConstantsMatch(constants,constants->revision,scope.storage,data,ConstantSnapshotReuseEnabled());
            }
            if(compareConstants&&!sameConstants&&constants&&last.constants) {
                const auto* previous=static_cast<const constant_state::Snapshot*>(last.constants.get());
                sameConstants=true;
                for(unsigned stage=0;stage<2&&sameConstants;++stage)
                    sameConstants=!std::memcmp(constants->floats[stage].data(),previous->floats[stage].data(),std::size_t(last.constantRows[stage])*16);
            }
            const bool sameGeometry=geometry&&lastGeometry&&geometry_bindings::SameBindings(*geometry,*lastGeometry);
            if(!sameSelection)++profile.missSelection;
            else if(!sameTextures)++profile.missTextures;
            else if(!sameConstants)++profile.missConstants;
            else if(!sameGeometry)++profile.missGeometry;
            if(sameSelection&&sameConstants&&sameTextures&&!sameGeometry) {
                const bool offsetOnly=geometry&&lastGeometry&&geometry_packet::OffsetOnly(*geometry,*lastGeometry);
                if(offsetOnly)++profile.geometryOffsetMisses;else ++profile.geometryBufferMisses;
                if(geometry_packet::Proven(GeometryPacketReuseEnabled(),last,scope.state,scope.pass.handle,
                    stamp,sameSelection,sameConstants,sameTextures,offsetOnly)) {
                    const bool drawn=geometry_packet::Record(scope.command,geometry,scope.packets,*scope.binds,
                        baseIndex,vertexCount,triCount,error);
                    if(drawn){last.geometry=geometry;++profile.geometryPacketHits;}
                    else last.valid=false;
                    Phase(profile.geometry,timing);++profile.draws;if(drawn)++profile.reused;return drawn;
                }
            }
            if(sameSelection&&sameConstants&&sameTextures&&sameGeometry) {
                bool drawn;
                if(GeometryRetentionReuseEnabled()) {
                    // last.geometry was installed only after an earlier Packet
                    // retained these GPU bindings in this frame's packet arena.
                    const auto frameRetained=std::static_pointer_cast<const geometry_bindings::Snapshot>(last.geometry);
                    drawn=geometry_bindings::DrawIndexedRetained(scope.command,geometry,frameRetained,scope.binds,baseIndex,vertexCount,triCount,error,true);
                } else {
                    std::vector<std::shared_ptr<const geometry_bindings::Snapshot>> retained;
                    geometry_bindings::RecordingScope recording(state,scope.command,retained,geometry,scope.binds);
                    drawn=geometry_bindings::DrawIndexed(state,baseIndex,vertexCount,triCount,error,true);
                }
                Phase(profile.geometry,timing);++profile.draws;++profile.reused;return drawn;
            }
        }
    }
    if(shader_dump::Enabled())shader_dump::Write(scope.state->material?scope.state->material->info.name:nullptr,
        scope.state->technique?scope.state->technique->name:nullptr,draw->selection->vertex->bytecode,draw->selection->pixel->bytecode,
        reinterpret_cast<const float(*)[4]>(draw->constants->floats[0].data()),reinterpret_cast<const float(*)[4]>(draw->constants->floats[1].data()));
    Phase(profile.capture,timing);
    std::shared_ptr<GraphicsProgram> program;
    if(scope.binds)program=scope.binds->prepared.Find(stamp,PreparedDrawReuseEnabled());
    if(program) {
        ++profile.programReuse;
    } else {
    ++profile.fullPrepares;
    std::vector<VkVertexInputBindingDescription> bindings;
    for(unsigned i=0;i<draw->geometry->streams.size();++i)if(draw->geometry->streams[i].buffer)
        bindings.push_back({i,draw->geometry->streams[i].stride,VK_VERTEX_INPUT_RATE_VERTEX});
    std::vector<VkVertexInputAttributeDescription> resolved;const std::vector<VkVertexInputAttributeDescription>* routed=&resolved;
    unsigned vertexUbyteMask=0;
    VertexResolveKey resolveKey{selected.declaration.get(),&selected.vertex->translation,selected.variant};
    for(const auto& binding:bindings){resolveKey.streamMask|=1u<<binding.binding;resolveKey.strides[binding.binding]=binding.stride;}
    const VertexResolveValue* cachedResolve=nullptr;
    if(scope.binds){auto found=scope.binds->vertexResolves.find(resolveKey);if(found!=scope.binds->vertexResolves.end())cachedResolve=&found->second;}
    if(cachedResolve) {routed=&cachedResolve->attributes;vertexUbyteMask=cachedResolve->ubyteMask;}
    else {
        if(!vertex_assets::Resolve(*selected.declaration,selected.variant,selected.vertex->translation,scope.physical,bindings,resolved,error,&vertexUbyteMask))return false;
        if(scope.binds)routed=&scope.binds->vertexResolves.emplace(resolveKey,VertexResolveValue{resolved,vertexUbyteMask}).first->second.attributes;
    }
    const auto& attributes=*routed;
    Phase(profile.resolve,timing);
    ProgramDescription d;d.vertexOwner=selected.vertex;d.pixelOwner=selected.pixel;d.vertexBytecode=selected.vertex->bytecode.data();d.vertexBytes=selected.vertex->bytecode.size()*4;
    d.vertexUbyteMask=vertexUbyteMask;
    d.pixelBytecode=selected.pixel->bytecode.data();d.pixelBytes=selected.pixel->bytecode.size()*4;
    d.pixelDepthSamplerMask=stamp.depthSamplerMask;
    d.renderPass=scope.pass.pipelineHandle?scope.pass.pipelineHandle:scope.pass.handle;d.subpass=scope.pass.subpass;d.samples=scope.pass.samples;d.colorAttachment=scope.pass.color;
    d.depthOnly=!scope.pass.color&&DepthOnlyPipelines();
    d.passKey=scope.pass.key;d.passKeyed=scope.pass.keyed;
    d.vertexBindings=bindings.data();d.vertexBindingCount=unsigned(bindings.size());
    d.vertexAttributes=attributes.data();d.vertexAttributeCount=unsigned(attributes.size());
    if(!ApplyMaterialState(draw->words[0],draw->words[1],d,error))return false;
    if((d.depthTest||d.depthWrite)&&!scope.pass.depth) {error="Native material requests depth without a depth attachment";return false;}
    if(d.stencilTest&&!scope.pass.stencil) {error="Native material requests stencil without a stencil attachment";return false;}
    if(!ResolveDepthBias(d,scope.pass.depthFormat,draw->polygonBias,draw->polygonSlope,error))return false;
    Phase(profile.state,timing);
    program=scope.cache.Acquire(d,error);if(!program)return false;
    if(scope.binds)scope.binds->prepared={stamp,draw->selection,program};
    }
    Phase(profile.program,timing);
    VkDeviceSize cursor=scope.cursor;std::vector<ProgramResource> localVs,localPs;
    ResourceScratchLease scratch(scope.binds,scope.storage&&ResourceScratchReuseEnabled());
    auto& vs=scratch.Get()?scratch.Get()->stages[0]:localVs;
    auto& ps=scratch.Get()?scratch.Get()->stages[1]:localPs;
    VkDescriptorPool pool=scope.pool;
    const unsigned rows[2]{program->ConstantRows(0),program->ConstantRows(1)};
    if(scope.storage) {
    const bool uploaded=scratch.Get()
        ?material_bindings::UploadScratch(*draw,*scope.storage,scope.alignment,*scratch.Get(),pool,error,rows,scope.binds)
        :material_bindings::Upload(*draw,*scope.storage,scope.alignment,vs,ps,pool,error,rows,scope.binds);
    if(!uploaded) {
        error+=" material="+std::string(scope.state->material?scope.state->material->info.name:"<none>")
            +" technique="+std::string(scope.state->technique?scope.state->technique->name:"<none>");return false;
    }}
    else if(!material_bindings::Upload(*draw,scope.uniforms,cursor,scope.alignment,vs,ps,error))return false;
    Phase(profile.upload,timing);
    Packet packet;packet.textures=draw->textures;packet.geometry=draw->geometry;packet.program=program;
    if(scope.binds) {
        // Frame-cached sets with dynamic constant offsets; unchanged bindings
        // are not re-recorded. Packet still retains every GPU owner.
        if(!program->Record(scope.command,pool,vs,ps,*scope.binds,error))return false;
        scope.packets.push_back(packet);scope.cursor=cursor;
    } else {
        if(!program->BindResources(pool,vs,ps,packet.sets,error))return false;
        // All shader/descriptor input has been validated before binding. Keep the
        // actual GPU owners even if subsequent geometry validation fails.
        // The CPU constant bank and shader-selection metadata can retire on return.
        scope.packets.push_back(packet);scope.cursor=cursor;
        program->Bind(scope.command,packet.sets);
    }
    Phase(profile.descriptors,timing);
    bool drawn;
    if(GeometryRetentionReuseEnabled()) {
        drawn=geometry_bindings::DrawIndexedRetained(scope.command,draw->geometry,scope.packets.back().geometry,scope.binds,baseIndex,vertexCount,triCount,error);
    } else {
        std::vector<std::shared_ptr<const geometry_bindings::Snapshot>> geometry;
        geometry_bindings::RecordingScope recording(state,scope.command,geometry,draw->geometry,scope.binds);
        drawn=geometry_bindings::DrawIndexed(state,baseIndex,vertexCount,triCount,error);
    }
    if(scope.binds) {
        auto& last=scope.binds->last;
        last.programStamp=stamp;
        last.valid=drawn;last.state=scope.state;last.pass=scope.pass.handle;
        last.words[0]=draw->words[0];last.words[1]=draw->words[1];last.revision=draw->constants->revision;
        last.selection=draw->selection;last.textures=draw->textures;last.geometry=draw->geometry;
        last.samplesTextures=program->SamplesTextures();
        const void* data[2]={draw->constants->floats[0].data(),draw->constants->floats[1].data()};
        scope.binds->RememberConstants(draw->constants,draw->constants->revision,scope.storage,data,rows,ConstantSnapshotReuseEnabled());
        if(last.constantStorage)++profile.weakConstants;else ++profile.strongConstants;
    }
    Phase(profile.geometry,timing);++profile.draws;return drawn;
}
}
