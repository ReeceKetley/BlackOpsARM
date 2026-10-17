#include "query_assets.h"
#include "pass_recording.h"
#include "pointer_hash.h"
#include <mutex>
#include <limits>
namespace bo1::vulkan::query_assets {
struct Entry {
 std::shared_ptr<OcclusionQueries> latest;unsigned latestIndex{}; // slot of the last issued query
 bool issued=false,cached=false;unsigned value{};std::uint64_t generation{};
};
namespace {
 std::mutex mutex;VkDevice device{};bool precise{};
 std::unordered_map<const void*,std::shared_ptr<Entry>,bo1::vulkan::PointerHash> owners;
 // Frame pools: each holds one slot per owner. Owners keep the pool of their
 // last result alive; framePools are reused once their submission retires.
 std::vector<std::shared_ptr<OcclusionQueries>> framePools;thread_local RecordingScope* current{};
 bool Drained(){
  if(owners.empty())for(auto it=framePools.begin();it!=framePools.end();)if(it->use_count()==1&&(*it)->Retired())it=framePools.erase(it);else ++it;
  return owners.empty()&&framePools.empty();
 }
 constexpr unsigned MaxOwners=4096;
 std::shared_ptr<OcclusionQueries> FramePool(unsigned required,std::string& error) {
  std::shared_ptr<OcclusionQueries> found;
  for(auto it=framePools.begin();it!=framePools.end();) {
   if(!(*it)->Retired()){++it;continue;}
   if((*it)->Capacity()<required){it=framePools.erase(it);continue;} // superseded by a larger pool
   if(!found)found=*it;++it;
  }
  if(found)return found;
  unsigned capacity=256;while(capacity<required)capacity*=2;if(capacity>MaxOwners)capacity=MaxOwners;
  auto pool=std::make_shared<OcclusionQueries>(device,precise);if(!pool->Create(capacity,error))return {};
  framePools.push_back(pool);return pool;
 }
 QueryRead ReadEntry(Entry& entry,unsigned& value,std::string& error) {
  if(!entry.issued)return QueryRead::Pending;
  if(entry.cached){value=entry.value;return QueryRead::Ready;}
  std::uint64_t samples{},generation{};auto status=entry.latest->Read(entry.latestIndex,samples,generation,error);
  if(status==QueryRead::Ready){if(samples>std::numeric_limits<unsigned>::max()){error="Native query sample count exceeds engine width";return QueryRead::Failed;}
   entry.value=unsigned(samples);entry.cached=true;value=entry.value;}
  return status;
 }
}
bool Empty(){std::lock_guard<std::mutex> lock(mutex);return Drained();}
bool Attach(VkDevice next,bool exact,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);if(next==device&&exact==precise)return true;
 if(!Drained()){error="Cannot change native query device with owners/frame borrowers";return false;}
 device=next;precise=next&&exact;return true;
}
bool Allocate(const void* owner,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);
 if(!precise){error="Native engine sample-count queries require enabled precise occlusion";return false;}
 if(!device||!owner||owners.count(owner)||owners.size()>=MaxOwners){error="Invalid native occlusion owner/context/capacity";return false;}
 owners.emplace(owner,std::make_shared<Entry>());return true;
}
bool Has(const void* owner){std::lock_guard<std::mutex> lock(mutex);return owners.count(owner)!=0;}
void Release(const void* owner){std::lock_guard<std::mutex> lock(mutex);owners.erase(owner);}
RecordingScope::RecordingScope(VkCommandBuffer c,Arena& a):command(c),arena(a),previous_(current){current=this;}
RecordingScope::~RecordingScope(){current=previous_;}
bool RecordingScope::Prepare(std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);if(!device||!command||!prepared.empty()){error="Invalid native query frame preparation";return false;}
 if(owners.empty())return true;
 auto pool=FramePool(unsigned(owners.size()),error);if(!pool)return false;
 // Results still held only by the pool about to be reset are read first.
 for(const auto& item:owners) {
  auto& entry=*item.second;unsigned old{};
  if(entry.issued&&!entry.cached&&entry.latest==pool&&ReadEntry(entry,old,error)==QueryRead::Failed)return false;
 }
 if(!pool->Reset(command,error))return false;
 arena.pools.push_back(pool);prepared.reserve(owners.size());
 unsigned index=0;for(const auto& item:owners)prepared.emplace(item.first,Prepared{item.second,pool,index++});
 return true;
}
bool RecordingScope::Submitted(VkFence fence,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);
 if(active){error="Native query frame submitted with an active query";return false;}
 // Every prepared owner shares the frame pool: publish it once.
 if(!prepared.empty()&&!prepared.begin()->second.pool->Submitted(fence,error))return false;return true;
}
// Inside a pass whose body is recorded into secondary command buffers the
// query brackets its draws in the recording thread's current secondary.
static VkCommandBuffer QueryCommand(VkCommandBuffer frame) {
 const auto* passes=pass_recording::Current();return passes&&passes->Command()?passes->Command():frame;
}
bool Begin(const void* owner,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);
 if(!current||current->active){error="Native query begin needs a frame scope without nested occlusion";return false;}
 auto found=current->prepared.find(owner);auto registered=owners.find(owner);
 if(found==current->prepared.end()||registered==owners.end()||registered->second!=found->second.entry){error="Native query owner was not prepared for this frame";return false;}
 auto& entry=*found->second.entry;
 if(auto* passes=pass_recording::Current()){passes->FlushPendingDraw();if(!passes->OpenForQuery(error))return false;} // queries bracket the actual draws
 if(entry.generation==std::numeric_limits<std::uint64_t>::max()||!found->second.pool->Begin(QueryCommand(current->command),found->second.index,error))return false;
 entry.latest=found->second.pool;entry.latestIndex=found->second.index;entry.issued=true;entry.cached=false;++entry.generation;current->active=owner;return true;
}
bool OcclusionOpen(){std::lock_guard<std::mutex> lock(mutex);return current&&current->active;}
bool End(const void* owner,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);
 if(!current||current->active!=owner){error="Native query end has no matching owner";return false;}
 if(auto* passes=pass_recording::Current())passes->FlushPendingDraw();
 auto found=current->prepared.find(owner);if(found==current->prepared.end()||!found->second.pool->End(QueryCommand(current->command),found->second.index,error))return false;
 current->active=nullptr;return true;
}
QueryRead Read(const void* owner,unsigned& samples,bool flush,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);auto found=owners.find(owner);
 if(found==owners.end()){error="Native query read owner is missing";return QueryRead::Failed;}
 auto& entry=*found->second;
 if(current&&current->active==owner)return QueryRead::Pending;
 if(flush&&entry.issued&&!entry.cached&&!entry.latest->Published()){error="Native query flush requires engine frame submission";return QueryRead::Failed;}
 return ReadEntry(entry,samples,error);
}
}
