#include "material_dispatch.h"
#include "material_state.h"
#include "shader_dump.h"
#include <chrono>
#include <cstring>
#include "gfx_d3d/rb_state.h"
namespace bo1::vulkan::material_dispatch {
namespace {thread_local RecordingScope* current{};thread_local Profile profile;thread_local bool timing{};
// Per-draw phase timing reads the clock eight times per draw; only frames whose
// profile is actually reported pay for it.
std::uint64_t Now(){return timing?std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count():0;}
void Phase(std::uint64_t& field,std::uint64_t& start){if(!timing)return;const auto now=Now();field+=now-start;start=now;}
}
void ResetProfile(bool timed){profile={};timing=timed;}Profile ReadProfile(){return profile;}
RecordingScope::RecordingScope(const GfxCmdBufState* s,VkCommandBuffer c,VkPhysicalDevice p,Pass target,
    ProgramCache& pc,Buffer& u,VkDescriptorPool dp,VkDeviceSize& offset,VkDeviceSize align,std::vector<Packet>& retained,DrawStorage* pages,BindCache* bindCache)
    :state(s),command(c),physical(p),pass(target),cache(pc),uniforms(u),pool(dp),cursor(offset),alignment(align),packets(retained),storage(pages),binds(bindCache),previous_(current){current=this;}
RecordingScope::~RecordingScope(){current=previous_;}
bool HasScope(){return current!=nullptr;}
bool DrawIndexed(const GfxCmdBufPrimState* state,int baseIndex,int vertexCount,int triCount,std::string& error) {
    if(!current||!current->state||&current->state->prim!=state||!current->command||!current->physical||!current->pass.handle||!current->pool) {
        error="Native material draw has no matching pass/frame recording scope";return false;
    }
    auto& scope=*current;auto timing=Now();
    if(scope.binds&&scope.binds->last.valid) {
        // D3D9 runtimes absorbed redundant re-issued state; do the same here:
        // a draw identical to the previous one except for its index range
        // reuses the already bound pipeline, descriptor sets and buffers.
        auto& last=scope.binds->last;
        if(!(last.state==scope.state&&last.pass==scope.pass.handle
            &&last.words[0]==scope.state->activeStateBits[0]&&last.words[1]==scope.state->activeStateBits[1]))++profile.missState;
        else {
            const auto selection=material_bindings::Acquire(scope.state);const auto constants=constant_state::Acquire(scope.state);
            const auto textures=texture_bindings::Acquire(scope.state);const auto geometry=geometry_bindings::Acquire(state);
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
            bool sameConstants=constants&&constants==last.constants&&constants->revision==last.revision;
            if(!sameConstants&&constants&&last.constants) {
                const auto* previous=static_cast<const constant_state::Snapshot*>(last.constants.get());
                sameConstants=true;
                for(unsigned stage=0;stage<2&&sameConstants;++stage)
                    sameConstants=!std::memcmp(constants->floats[stage].data(),previous->floats[stage].data(),std::size_t(last.constantRows[stage])*16);
            }
            const bool sameGeometry=geometry&&lastGeometry&&geometry_bindings::SameBindings(*geometry,*lastGeometry);
            if(!sameSelection)++profile.missSelection;
            else if(!sameConstants)++profile.missConstants;
            else if(!sameTextures)++profile.missTextures;
            else if(!sameGeometry)++profile.missGeometry;
            if(sameSelection&&sameConstants&&sameTextures&&sameGeometry) {
                std::vector<std::shared_ptr<const geometry_bindings::Snapshot>> retained;
                geometry_bindings::RecordingScope recording(state,scope.command,retained,geometry,scope.binds);
                const bool drawn=geometry_bindings::DrawIndexed(state,baseIndex,vertexCount,triCount,error,true);
                Phase(profile.geometry,timing);++profile.draws;++profile.reused;return drawn;
            }
        }
    }
    auto draw=material_bindings::Capture(scope.state,error);if(!draw)return false;
    if(shader_dump::Enabled())shader_dump::Write(scope.state->material?scope.state->material->info.name:nullptr,
        scope.state->technique?scope.state->technique->name:nullptr,draw->selection->vertex->bytecode,draw->selection->pixel->bytecode,
        reinterpret_cast<const float(*)[4]>(draw->constants->floats[0].data()),reinterpret_cast<const float(*)[4]>(draw->constants->floats[1].data()));
    Phase(profile.capture,timing);
    std::vector<VkVertexInputBindingDescription> bindings;
    for(unsigned i=0;i<draw->geometry->streams.size();++i)if(draw->geometry->streams[i].buffer)
        bindings.push_back({i,draw->geometry->streams[i].stride,VK_VERTEX_INPUT_RATE_VERTEX});
    std::vector<VkVertexInputAttributeDescription> resolved;const std::vector<VkVertexInputAttributeDescription>* routed=&resolved;
    const auto& selected=*draw->selection;unsigned vertexUbyteMask=0;
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
    // The specialization and sampler come from the same immutable capture.
    // Unused bound textures do not create extra shader variants.
    for(const auto& binding:selected.pixel->translation.bindings) {
        if(binding.kind!=ResourceKind::Sampler)continue;
        if(binding.registerIndex>=16||!draw->textures) {error="Native sampler capture missing";return false;}
        const auto& sampled=draw->textures->slots[binding.registerIndex];
        if(sampled.image&&sampled.image->depthComparison) {
            if(!sampled.sampler||!sampled.sampler->Comparison()) {error="Native shadow image lacks its comparison sampler";return false;}
            d.pixelDepthSamplerMask|=1u<<binding.registerIndex;
        }
    }
    d.renderPass=scope.pass.pipelineHandle?scope.pass.pipelineHandle:scope.pass.handle;d.subpass=scope.pass.subpass;d.samples=scope.pass.samples;d.colorAttachment=scope.pass.color;
    d.vertexBindings=bindings.data();d.vertexBindingCount=unsigned(bindings.size());
    d.vertexAttributes=attributes.data();d.vertexAttributeCount=unsigned(attributes.size());
    if(!ApplyMaterialState(draw->words[0],draw->words[1],d,error))return false;
    if((d.depthTest||d.depthWrite)&&!scope.pass.depth) {error="Native material requests depth without a depth attachment";return false;}
    if(d.stencilTest&&!scope.pass.stencil) {error="Native material requests stencil without a stencil attachment";return false;}
    if(!ResolveDepthBias(d,scope.pass.depthFormat,draw->polygonBias,draw->polygonSlope,error))return false;
    Phase(profile.state,timing);
    auto program=scope.cache.Acquire(d,error);if(!program)return false;
    Phase(profile.program,timing);
    VkDeviceSize cursor=scope.cursor;std::vector<ProgramResource> vs,ps;
    VkDescriptorPool pool=scope.pool;
    const unsigned rows[2]{program->ConstantRows(0),program->ConstantRows(1)};
    if(scope.storage) {if(!material_bindings::Upload(*draw,*scope.storage,scope.alignment,vs,ps,pool,error,rows,scope.binds)) {
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
    std::vector<std::shared_ptr<const geometry_bindings::Snapshot>> geometry;
    geometry_bindings::RecordingScope recording(state,scope.command,geometry,draw->geometry,scope.binds);
    const bool drawn=geometry_bindings::DrawIndexed(state,baseIndex,vertexCount,triCount,error);
    if(scope.binds) {
        auto& last=scope.binds->last;
        last.valid=drawn;last.state=scope.state;last.pass=scope.pass.handle;
        last.words[0]=draw->words[0];last.words[1]=draw->words[1];last.revision=draw->constants->revision;
        last.selection=draw->selection;last.constants=draw->constants;last.textures=draw->textures;last.geometry=draw->geometry;
        last.samplesTextures=program->SamplesTextures();
        last.constantRows[0]=program->ConstantRows(0);last.constantRows[1]=program->ConstantRows(1);
    }
    Phase(profile.geometry,timing);++profile.draws;return drawn;
}
}
