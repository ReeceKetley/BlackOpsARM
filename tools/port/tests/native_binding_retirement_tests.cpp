#include "gfx_vulkan/sharded_states.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

static std::atomic<unsigned> destroyed{};
struct Snapshot { ~Snapshot(){++destroyed;} };
static unsigned checks;
static void check(bool ok){if(!ok)std::abort();++checks;}

int main(){
    // Distinct producers publish command state concurrently. Retirement starts
    // only after join, matching the device's quiescent shutdown contract.
    bo1::vulkan::ShardedStates<const void*,std::shared_ptr<Snapshot>> states;
    int keys[4096]{};
    std::weak_ptr<Snapshot> weak[4096];
    std::vector<std::thread> producers;
    for(unsigned t=0;t<8;++t)producers.emplace_back([&,t]{
        for(unsigned i=t;i<4096;i+=8){
            auto snapshot=std::make_shared<Snapshot>();weak[i]=snapshot;
            auto& shard=states.For(&keys[i]);std::lock_guard<std::mutex> lock(shard.mutex);
            shard.map[&keys[i]]=std::move(snapshot);
        }
    });
    for(auto& thread:producers)thread.join();
    std::vector<std::shared_ptr<Snapshot>> submitted;
    for(unsigned i=0;i<4096;i+=257)submitted.push_back(weak[i].lock());
    check(destroyed==0);check(states.RetireAll()==4096);
    // Releasing registry borrowers must not destroy a still-retained snapshot.
    for(unsigned i=0;i<4096;++i)check(weak[i].expired()==(i%257!=0));
    check(destroyed==4096-submitted.size());
    submitted.clear();check(destroyed==4096);
    for(const auto& entry:weak)check(entry.expired());
    check(states.RetireAll()==0);
    // The registry remains usable for a later device owner; no stale keys.
    {auto& shard=states.For(&keys[0]);std::lock_guard<std::mutex> lock(shard.mutex);
        check(shard.map.empty());shard.map[&keys[0]]=std::make_shared<Snapshot>();}
    check(states.RetireAll()==1);check(destroyed==4097);
    std::printf("binding retirement %u checks PASS\n",checks);
}
