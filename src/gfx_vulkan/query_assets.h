#pragma once
#include "occlusion_queries.h"
#include "pointer_hash.h"
#include <memory>
#include <unordered_map>
namespace bo1::vulkan::query_assets {
bool Attach(VkDevice,bool precise,std::string&);bool Empty();
bool Allocate(const void* owner,std::string&);bool Has(const void*);void Release(const void*);
bool Begin(const void*,std::string&);bool End(const void*,std::string&);
QueryRead Read(const void*,unsigned& samples,bool flush,std::string&);
struct Arena {std::vector<std::shared_ptr<OcclusionQueries>> pools;void Retire(){pools.clear();}};
struct Entry;
// Prepare resets outside passes. Registry keeps the previous issued result
// until Begin selects the prepared backing; frame arena retains GPU users.
class RecordingScope {
public:
 RecordingScope(VkCommandBuffer,Arena&);~RecordingScope();
 RecordingScope(const RecordingScope&)=delete;RecordingScope& operator=(const RecordingScope&)=delete;
 bool Prepare(std::string&);bool Submitted(VkFence,std::string&);
 struct Prepared {std::shared_ptr<Entry> entry;std::shared_ptr<OcclusionQueries> pool;};
 VkCommandBuffer command;Arena& arena;std::unordered_map<const void*,Prepared,bo1::vulkan::PointerHash> prepared;const void* active{};
private:RecordingScope* previous_;
};
}
