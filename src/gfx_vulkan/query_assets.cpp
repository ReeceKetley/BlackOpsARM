#include "query_assets.h"
#include "pass_recording.h"
#include "pointer_hash.h"
#include <mutex>
#include <limits>
namespace bo1::vulkan::query_assets {
struct Entry {
 std::vector<std::shared_ptr<OcclusionQueries>> backings;std::shared_ptr<OcclusionQueries> latest;
 bool issued=false,cached=false;unsigned value{};std::uint64_t generation{};
};
namespace {
 std::mutex mutex;VkDevice device{};bool precise{};
 std::unordered_map<const void*,std::shared_ptr<Entry>,bo1::vulkan::PointerHash> owners;
 std::vector<std::weak_ptr<OcclusionQueries>> live;thread_local RecordingScope* current{};
 bool Drained(){for(auto it=live.begin();it!=live.end();)if(it->expired())it=live.erase(it);else ++it;return owners.empty()&&live.empty();}
 std::shared_ptr<OcclusionQueries> New(std::string& error) {
  for(auto it=live.begin();it!=live.end();)if(it->expired())it=live.erase(it);else ++it;
  auto pool=std::make_shared<OcclusionQueries>(device,precise);if(!pool->Create(1,error))return {};
  live.push_back(pool);return pool;
 }
 QueryRead ReadEntry(Entry& entry,unsigned& value,std::string& error) {
  if(!entry.issued)return QueryRead::Pending;
  if(entry.cached){value=entry.value;return QueryRead::Ready;}
  std::uint64_t samples{},generation{};auto status=entry.latest->Read(0,samples,generation,error);
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
 if(!device||!owner||owners.count(owner)||owners.size()>=4096){error="Invalid native occlusion owner/context/capacity";return false;}
 auto pool=New(error);if(!pool)return false;auto entry=std::make_shared<Entry>();entry->backings.push_back(pool);entry->latest=pool;owners.emplace(owner,entry);return true;
}
bool Has(const void* owner){std::lock_guard<std::mutex> lock(mutex);return owners.count(owner)!=0;}
void Release(const void* owner){std::lock_guard<std::mutex> lock(mutex);owners.erase(owner);}
RecordingScope::RecordingScope(VkCommandBuffer c,Arena& a):command(c),arena(a),previous_(current){current=this;}
RecordingScope::~RecordingScope(){current=previous_;}
bool RecordingScope::Prepare(std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);if(!device||!command||!prepared.empty()){error="Invalid native query frame preparation";return false;}
 for(const auto& item:owners) {
  auto& entry=*item.second;unsigned old{};
  if(entry.issued&&!entry.cached&&entry.latest->Retired()&&ReadEntry(entry,old,error)==QueryRead::Failed)return false;
  std::shared_ptr<OcclusionQueries> pool;
  for(auto& candidate:entry.backings)if(candidate->Retired()){pool=candidate;break;}
  if(!pool){pool=New(error);if(!pool)return false;entry.backings.push_back(pool);}
  if(!pool->Reset(command,error))return false;
  arena.pools.push_back(pool);prepared.emplace(item.first,Prepared{item.second,pool});
 }
 return true;
}
bool RecordingScope::Submitted(VkFence fence,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);
 if(active){error="Native query frame submitted with an active query";return false;}
 for(auto& item:prepared)if(!item.second.pool->Submitted(fence,error))return false;return true;
}
bool Begin(const void* owner,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);
 if(!current||current->active){error="Native query begin needs a frame scope without nested occlusion";return false;}
 auto found=current->prepared.find(owner);auto registered=owners.find(owner);
 if(found==current->prepared.end()||registered==owners.end()||registered->second!=found->second.entry){error="Native query owner was not prepared for this frame";return false;}
 auto& entry=*found->second.entry;
 if(auto* passes=pass_recording::Current())passes->FlushPendingDraw(); // queries bracket the actual draws
 if(entry.generation==std::numeric_limits<std::uint64_t>::max()||!found->second.pool->Begin(current->command,0,error))return false;
 entry.latest=found->second.pool;entry.issued=true;entry.cached=false;++entry.generation;current->active=owner;return true;
}
bool End(const void* owner,std::string& error) {
 std::lock_guard<std::mutex> lock(mutex);
 if(!current||current->active!=owner){error="Native query end has no matching owner";return false;}
 if(auto* passes=pass_recording::Current())passes->FlushPendingDraw();
 auto found=current->prepared.find(owner);if(found==current->prepared.end()||!found->second.pool->End(current->command,0,error))return false;
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
