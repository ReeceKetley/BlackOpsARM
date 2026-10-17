#pragma once
#include "graphics_program.h"
namespace bo1::vulkan {
// CPU descriptor descriptions only. GPU lifetimes remain with packets/storage.
struct ResourceScratch {std::vector<ProgramResource> stages[2],added;bool leased{};};
struct ResourceScratchPool {std::vector<std::unique_ptr<ResourceScratch>> slots;};
struct ResourceScratchProfile {std::uint64_t leases{},nestedLeases{},capacityGrowths{};};
inline thread_local ResourceScratchProfile resourceScratchProfile;
class ResourceScratchLease {
    std::shared_ptr<ResourceScratchPool> pool_;ResourceScratch* scratch_{};
    std::array<std::size_t,3> capacities_{};
public:
    ResourceScratchLease(BindCache* cache,bool enabled) {
        if(!enabled||!cache)return;
        if(!cache->resourceScratch)cache->resourceScratch=std::make_shared<ResourceScratchPool>();
        pool_=cache->resourceScratch;bool nested=false;
        for(const auto& slot:pool_->slots){nested|=slot->leased;if(!slot->leased&&!scratch_)scratch_=slot.get();}
        if(!scratch_){pool_->slots.push_back(std::make_unique<ResourceScratch>());scratch_=pool_->slots.back().get();}
        scratch_->leased=true;++resourceScratchProfile.leases;if(nested)++resourceScratchProfile.nestedLeases;
        capacities_={scratch_->stages[0].capacity(),scratch_->stages[1].capacity(),scratch_->added.capacity()};
        scratch_->stages[0].clear();scratch_->stages[1].clear();scratch_->added.clear();
    }
    ~ResourceScratchLease(){if(!scratch_)return;
        resourceScratchProfile.capacityGrowths+=(scratch_->stages[0].capacity()>capacities_[0])
            +(scratch_->stages[1].capacity()>capacities_[1])+(scratch_->added.capacity()>capacities_[2]);
        scratch_->stages[0].clear();scratch_->stages[1].clear();scratch_->added.clear();scratch_->leased=false;
    }
    ResourceScratchLease(const ResourceScratchLease&)=delete;
    ResourceScratchLease& operator=(const ResourceScratchLease&)=delete;
    ResourceScratch* Get()const{return scratch_;}
};
}
