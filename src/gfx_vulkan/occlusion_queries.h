#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <string>
namespace bo1::vulkan {
enum class QueryRead {Ready,Pending,Failed};
// One fence-retired frame batch. Reset records outside a render pass; Begin/End
// surround draws. Submitted is called ONLY after successful queue submission,
// with that submission's reset-before-submit fence. Caller retires GPU users
// before destruction or reset; CPU copies returned by Read include generation.
// Without Precise(), values represent zero/nonzero visibility only, and must
// never be used as calibrated sample counts.
class OcclusionQueries {
public:
    OcclusionQueries(VkDevice device,bool precise):device_(device),precise_(precise){}
    ~OcclusionQueries();OcclusionQueries(const OcclusionQueries&)=delete;OcclusionQueries& operator=(const OcclusionQueries&)=delete;
    bool Create(unsigned count,std::string&);
    bool Reset(VkCommandBuffer,std::string&);
    bool Begin(VkCommandBuffer,unsigned,std::string&);bool End(VkCommandBuffer,unsigned,std::string&);
    bool Submitted(VkFence,std::string&);
    QueryRead Read(unsigned,std::uint64_t& samples,std::uint64_t& generation,std::string&);
    bool Retired()const;
    bool Published()const{return phase_==Phase::Submitted;}
    bool Precise()const{return precise_;}
    unsigned Capacity()const{return unsigned(results_.size());}
private:
    struct Result {bool ended{},cached{};std::uint64_t value{};};
    VkDevice device_{};VkQueryPool pool_{};bool precise_{};VkCommandBuffer command_{};VkFence fence_{};
    enum class Phase {Idle,Recording,Submitted};Phase phase_=Phase::Idle;
    unsigned active_=~0u;std::uint64_t generation_{};std::vector<Result> results_;
};
}
