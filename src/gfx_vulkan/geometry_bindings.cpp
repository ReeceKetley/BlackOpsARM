#include "geometry_bindings.h"
#include "pointer_hash.h"
#include <mutex>
#include <unordered_map>
#include <limits>
namespace bo1::vulkan::geometry_bindings {
namespace {
std::mutex mutex;std::unordered_map<const GfxCmdBufPrimState*,std::shared_ptr<const Snapshot>,bo1::vulkan::PointerHash> states;
thread_local RecordingScope* current{};
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
    std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);
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
bool SetIndex(const GfxCmdBufPrimState* state,const void* owner,std::string& error) {
    auto buffer=buffer_assets::Acquire(owner);
    if(!state||!buffer||buffer->kind!=buffer_assets::Kind::Index16) {error="Invalid native index owner/kind";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);
    auto next=it==states.end()?std::make_shared<Snapshot>():std::make_shared<Snapshot>(*it->second);
    next->indices=std::move(buffer);states[state]=std::move(next);return true;
}
bool ClearVertices(const GfxCmdBufPrimState* state,std::string& error) {
    if(!state){error="Invalid native stream-clear command owner";return false;}
    std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);
    if(it==states.end())return true;
    auto next=std::make_shared<Snapshot>(*it->second);
    next->streams={};it->second=std::move(next);return true;
}
void Clone(const GfxCmdBufPrimState* source,const GfxCmdBufPrimState* destination) {
    std::lock_guard<std::mutex> lock(mutex);if(source==destination)return;auto it=states.find(source);
    if(it==states.end())states.erase(destination);else states[destination]=it->second;
}
void Release(const GfxCmdBufPrimState* state){std::lock_guard<std::mutex> lock(mutex);states.erase(state);}
std::shared_ptr<const Snapshot> Acquire(const GfxCmdBufPrimState* state) {std::lock_guard<std::mutex> lock(mutex);auto it=states.find(state);return it==states.end()?nullptr:it->second;}
RecordingScope::RecordingScope(const GfxCmdBufPrimState* s,VkCommandBuffer c,std::vector<std::shared_ptr<const Snapshot>>& a,std::shared_ptr<const Snapshot> snapshot,BindCache* bindCache):captured(std::move(snapshot)),state(s),command(c),arena(a),binds(bindCache),previous_(current){current=this;}
RecordingScope::~RecordingScope(){current=previous_;}
bool SameBindings(const Snapshot& a,const Snapshot& b) {
    if(!a.indices||!b.indices||a.indices->gpu!=b.indices->gpu)return false;
    for(unsigned i=0;i<a.streams.size();++i) {
        const auto& x=a.streams[i];const auto& y=b.streams[i];
        if(bool(x.buffer)!=bool(y.buffer))return false;
        if(x.buffer&&(x.buffer->gpu!=y.buffer->gpu||x.offset!=y.offset||x.stride!=y.stride))return false;
    }
    return true;
}
bool DrawIndexed(const GfxCmdBufPrimState* state,int baseIndex,int vertexCount,int triCount,std::string& error,bool mergeable) {
    if(!current||current->state!=state||!current->command) {error="Native indexed draw has no matching recording scope";return false;}
    if(baseIndex<0||vertexCount<=0||triCount<0||std::uint64_t(triCount)*3>std::numeric_limits<unsigned>::max()) {error="Invalid native indexed draw arguments";return false;}
    if(!triCount)return true;
    auto snapshot=current->captured?current->captured:Acquire(state);
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
    current->arena.push_back(snapshot);
    auto* binds=current->binds;
    for(unsigned i=0;i<snapshot->streams.size();++i)if(snapshot->streams[i].buffer) {
        const auto& stream=snapshot->streams[i];const VkBuffer buffer=stream.buffer->gpu->Handle();
        if(binds&&binds->vertexBuffers[i]==buffer&&binds->vertexOffsets[i]==stream.offset)continue;
        if(binds)FlushPending(*binds); // the pending draw uses the old binding
        vkCmdBindVertexBuffers(current->command,i,1,&buffer,&stream.offset);
        if(binds){binds->vertexBuffers[i]=buffer;binds->vertexOffsets[i]=stream.offset;}
    }
    const VkBuffer indices=snapshot->indices->gpu->Handle();
    if(!binds||binds->indexBuffer!=indices) {
        if(binds)FlushPending(*binds);
        vkCmdBindIndexBuffer(current->command,indices,0,VK_INDEX_TYPE_UINT16);
        if(binds)binds->indexBuffer=indices;
    }
    if(!binds){vkCmdDrawIndexed(current->command,unsigned(count),1,unsigned(baseIndex),0,0);return true;}
    auto& pending=binds->pending;
    if(mergeable&&pending.active&&pending.command==current->command&&unsigned(baseIndex)==pending.first+pending.count
        &&count<=std::uint64_t(std::numeric_limits<std::uint32_t>::max()-pending.count)) {pending.count+=unsigned(count);return true;}
    FlushPending(*binds);pending={true,current->command,unsigned(baseIndex),unsigned(count)};return true;
}
}
