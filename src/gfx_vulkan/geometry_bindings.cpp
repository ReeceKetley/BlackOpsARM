#include "geometry_bindings.h"
#include "sharded_states.h"
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <limits>
namespace bo1::vulkan::geometry_bindings {
namespace {
ShardedStates<const GfxCmdBufPrimState*,std::shared_ptr<const Snapshot>> registry;
thread_local RecordingScope* current{};
thread_local RecordingProfile profile;
std::atomic<bool> indexBindingReuse{false};
thread_local IndexBindingProfile indexProfile;
}
bool SetStreams(const GfxCmdBufPrimState* state,const std::array<StreamSource,3>& sources,std::string& error) {
    if(!state||!sources[0].owner){error="Native vertex stream set requires its command and primary owner";return false;}
    std::array<Stream,3> streams{};
    for(unsigned i=0;i<streams.size();++i) {
        const auto& source=sources[i];
        if(!source.owner) {
            if(source.offset||source.stride){error="Absent native stream has a nonzero range";return false;}
            continue;
        }
        auto buffer=buffer_assets::Acquire(source.owner);
        if(!buffer||buffer->kind!=buffer_assets::Kind::Vertex||!buffer->gpu||!buffer->gpu->Handle()
            ||!source.stride||source.offset>=buffer->payloadBytes||source.stride>buffer->payloadBytes-source.offset) {
            error="Invalid native vertex stream at slot "+std::to_string(i)+" owner="+std::to_string(reinterpret_cast<std::uintptr_t>(source.owner))
                +" present="+std::to_string(bool(buffer))+" payload="+std::to_string(buffer?buffer->payloadBytes:0)
                +" offset="+std::to_string(source.offset)+" stride="+std::to_string(source.stride);return false;
        }
        streams[i]={std::move(buffer),source.offset,source.stride};
    }
    auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);
    if(it!=states.end()) {
        bool same=true;for(unsigned i=0;i<streams.size();++i) {
            const auto& old=it->second->streams[i];const auto& next=streams[i];
            same=same&&old.buffer==next.buffer&&old.offset==next.offset&&old.stride==next.stride;
        }
        if(same)return true;
    }
    auto next=it==states.end()?std::make_shared<Snapshot>():std::make_shared<Snapshot>(*it->second);
    next->streams=std::move(streams);states[state]=std::move(next);return true;
}
bool SetVertex(const GfxCmdBufPrimState* state,const void* owner,unsigned offset,unsigned stride,std::string& error) {
    return SetStreams(state,{{{owner,offset,stride},{},{}}},error);
}
void SetIndexBindingReuse(bool value){indexBindingReuse.store(value,std::memory_order_relaxed);}
bool IndexBindingReuseEnabled(){return indexBindingReuse.load(std::memory_order_relaxed);}
IndexBindingProfile ReadIndexBindingProfile(){return indexProfile;}
bool SetIndex(const GfxCmdBufPrimState* state,const void* owner,std::string& error) {
    ++indexProfile.attempts;
    auto buffer=buffer_assets::Acquire(owner);
    if(!state||!buffer||buffer->kind!=buffer_assets::Kind::Index16) {error="Invalid native index owner/kind";return false;}
    auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);
    if(IndexBindingReuseEnabled()&&it!=states.end()&&it->second->indices==buffer){++indexProfile.hits;return true;}
    auto next=it==states.end()?std::make_shared<Snapshot>():std::make_shared<Snapshot>(*it->second);
    next->indices=std::move(buffer);states[state]=std::move(next);return true;
}
bool ClearVertices(const GfxCmdBufPrimState* state,std::string& error) {
    if(!state){error="Invalid native stream-clear command owner";return false;}
    auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);
    if(it==states.end())return true;
    auto next=std::make_shared<Snapshot>(*it->second);
    next->streams={};it->second=std::move(next);return true;
}
void Clone(const GfxCmdBufPrimState* source,const GfxCmdBufPrimState* destination) {
    if(source==destination)return;
    std::shared_ptr<const Snapshot> shared;
    {auto& shard=registry.For(source);std::lock_guard<std::mutex> lock(shard.mutex);auto it=shard.map.find(source);if(it!=shard.map.end())shared=it->second;}
    auto& shard=registry.For(destination);std::lock_guard<std::mutex> lock(shard.mutex);
    if(!shared)shard.map.erase(destination);else shard.map[destination]=std::move(shared);
}
void Release(const GfxCmdBufPrimState* state){auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;states.erase(state);}
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufPrimState* state) {auto& shard=registry.For(state);std::lock_guard<std::mutex> lock(shard.mutex);auto& states=shard.map;auto it=states.find(state);return it==states.end()?nullptr:it->second;}
RecordingScope::RecordingScope(const GfxCmdBufPrimState* s,VkCommandBuffer c,std::vector<std::shared_ptr<const Snapshot>>& a,std::shared_ptr<const Snapshot> snapshot,BindCache* bindCache):captured(std::move(snapshot)),state(s),command(c),arena(a),binds(bindCache),previous_(current){current=this;}
RecordingScope::~RecordingScope(){current=previous_;}
bool SameBindings(const Snapshot& a,const Snapshot& b) {
    if(!a.indices||!b.indices||a.indices->gpu!=b.indices->gpu||a.indices->offset!=b.indices->offset)return false;
    for(unsigned i=0;i<a.streams.size();++i) {
        const auto& x=a.streams[i];const auto& y=b.streams[i];
        if(bool(x.buffer)!=bool(y.buffer))return false;
        if(x.buffer&&(x.buffer->gpu!=y.buffer->gpu||x.buffer->offset+x.offset!=y.buffer->offset+y.offset||x.stride!=y.stride))return false;
    }
    return true;
}
static bool RecordCaptured(VkCommandBuffer command,const std::shared_ptr<const Snapshot>& snapshot,
    std::vector<std::shared_ptr<const Snapshot>>* arena,BindCache* binds,int baseIndex,int vertexCount,int triCount,std::string& error,bool mergeable) {
    if(!command){error="Native indexed draw has no command buffer";return false;}
    if(baseIndex<0||vertexCount<=0||triCount<0||std::uint64_t(triCount)*3>std::numeric_limits<unsigned>::max()) {error="Invalid native indexed draw arguments";return false;}
    if(!triCount)return true;
    if(!snapshot||!snapshot->indices||!snapshot->streams[0].buffer) {error="Native indexed draw missing buffer bindings";return false;}
    const std::uint64_t start=std::uint64_t(baseIndex)*2,count=std::uint64_t(triCount)*3;
    const auto payload=snapshot->indices->payloadBytes;
    if(start>payload||count>(payload-start)/2) {error="Native draw exceeds index payload";return false;}
    for(const auto& stream:snapshot->streams)if(stream.buffer) {
        if(!stream.stride||stream.offset>stream.buffer->payloadBytes||std::uint64_t(vertexCount)>(stream.buffer->payloadBytes-stream.offset)/stream.stride) {
            error="Native draw exceeds declared vertex stream range";return false;
        }
    }
    // Preserve ownership before recording; byte ranges are validated before
    // any command. Index values follow the canonical producer's vertex bounds.
    if(arena) {
        const bool grows=arena->size()==arena->capacity();arena->push_back(snapshot);
        ++profile.arenaRetentions;if(grows)++profile.arenaGrowths;
    } else ++profile.retainedDraws;
    for(unsigned i=0;i<snapshot->streams.size();++i)if(snapshot->streams[i].buffer) {
        const auto& stream=snapshot->streams[i];const VkBuffer buffer=stream.buffer->gpu->Handle();
        const VkDeviceSize offset=stream.buffer->offset+stream.offset; // packed records start inside a shared buffer
        if(binds&&binds->vertexBuffers[i]==buffer&&binds->vertexOffsets[i]==offset)continue;
        if(binds)FlushPending(*binds); // the pending draw uses the old binding
        vkCmdBindVertexBuffers(command,i,1,&buffer,&offset);
        if(binds){binds->vertexBuffers[i]=buffer;binds->vertexOffsets[i]=offset;}
    }
    const VkBuffer indices=snapshot->indices->gpu->Handle();
    if(!binds||binds->indexBuffer!=indices) {
        if(binds)FlushPending(*binds);
        vkCmdBindIndexBuffer(command,indices,0,VK_INDEX_TYPE_UINT16);
        if(binds)binds->indexBuffer=indices;
    }
    // A packed index record starts inside its shared buffer: index from there.
    const std::uint64_t firstIndex=std::uint64_t(baseIndex)+snapshot->indices->offset/2;
    if(firstIndex>std::numeric_limits<std::uint32_t>::max()-count) {error="Native draw index range overflow";return false;}
    baseIndex=int(firstIndex);
    if(!binds){vkCmdDrawIndexed(command,unsigned(count),1,unsigned(baseIndex),0,0);return true;}
    auto& pending=binds->pending;
    if(mergeable&&pending.active&&pending.command==command&&unsigned(baseIndex)==pending.first+pending.count
        &&count<=std::uint64_t(std::numeric_limits<std::uint32_t>::max()-pending.count)) {pending.count+=unsigned(count);++binds->mergedRanges;return true;}
    FlushPending(*binds);pending={true,command,unsigned(baseIndex),unsigned(count)};return true;
}
void ResetRecordingProfile(){profile={};}
RecordingProfile ReadRecordingProfile(){return profile;}
bool DrawIndexed(const GfxCmdBufPrimState* state,int baseIndex,int vertexCount,int triCount,std::string& error,bool mergeable) {
    if(!current||current->state!=state||!current->command) {error="Native indexed draw has no matching recording scope";return false;}
    const auto snapshot=triCount>0?(current->captured?current->captured:Acquire(state)):nullptr;
    return RecordCaptured(current->command,snapshot,&current->arena,current->binds,baseIndex,vertexCount,triCount,error,mergeable);
}
bool DrawIndexedRetained(VkCommandBuffer command,const std::shared_ptr<const Snapshot>& captured,
    const std::shared_ptr<const Snapshot>& retained,BindCache* binds,int baseIndex,int vertexCount,int triCount,std::string& error,bool mergeable) {
    if(triCount&&(!captured||!retained||!SameBindings(*captured,*retained))) {
        error="Native indexed draw lacks matching frame-retained geometry";return false;
    }
    return RecordCaptured(command,captured,nullptr,binds,baseIndex,vertexCount,triCount,error,mergeable);
}

}
