#include "occlusion_queries.h"
#include <limits>
namespace bo1::vulkan {
OcclusionQueries::~OcclusionQueries(){if(pool_)vkDestroyQueryPool(device_,pool_,nullptr);}
bool OcclusionQueries::Retired()const {return phase_==Phase::Idle||(phase_==Phase::Submitted&&vkGetFenceStatus(device_,fence_)==VK_SUCCESS);}
bool OcclusionQueries::Create(unsigned count,std::string& error) {
    if(!device_||pool_||!count||count>4096){error="Invalid native occlusion batch size/owner";return false;}
    VkQueryPoolCreateInfo create{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};create.queryType=VK_QUERY_TYPE_OCCLUSION;create.queryCount=count;
    auto result=vkCreateQueryPool(device_,&create,nullptr,&pool_);
    if(result!=VK_SUCCESS){error="Native occlusion query pool: "+std::to_string(int(result));return false;}
    results_.resize(count);return true;
}
bool OcclusionQueries::Reset(VkCommandBuffer command,std::string& error) {
    if(!pool_||!command||active_!=~0u||generation_==std::numeric_limits<std::uint64_t>::max()){error="Invalid native occlusion reset";return false;}
    if(phase_==Phase::Submitted){auto status=vkGetFenceStatus(device_,fence_);if(status!=VK_SUCCESS){error="Cannot reuse native query pool before its frame retires";return false;}}
    if(phase_==Phase::Recording){error="Native query recording must submit before reset/reuse";return false;}
    vkCmdResetQueryPool(command,pool_,0,unsigned(results_.size()));results_.assign(results_.size(),{});
    command_=command;fence_=VK_NULL_HANDLE;phase_=Phase::Recording;++generation_;return true;
}
bool OcclusionQueries::Begin(VkCommandBuffer command,unsigned query,std::string& error) {
    if(phase_!=Phase::Recording||command!=command_||query>=results_.size()||active_!=~0u||results_[query].ended){error="Invalid or repeated native query begin";return false;}
    vkCmdBeginQuery(command,pool_,query,precise_?VK_QUERY_CONTROL_PRECISE_BIT:0);active_=query;return true;
}
bool OcclusionQueries::End(VkCommandBuffer command,unsigned query,std::string& error) {
    if(phase_!=Phase::Recording||command!=command_||query!=active_||query>=results_.size()){error="Native query end has no matching begin";return false;}
    vkCmdEndQuery(command,pool_,query);results_[query].ended=true;active_=~0u;return true;
}
bool OcclusionQueries::Submitted(VkFence fence,std::string& error) {
    if(phase_!=Phase::Recording||!fence||active_!=~0u){error="Invalid native query submission publication";return false;}
    fence_=fence;phase_=Phase::Submitted;return true;
}
QueryRead OcclusionQueries::Read(unsigned query,std::uint64_t& samples,std::uint64_t& generation,std::string& error) {
    if(query>=results_.size()||!results_[query].ended){error="Native query result was not recorded";return QueryRead::Failed;}
    if(phase_!=Phase::Submitted)return QueryRead::Pending;
    auto& result=results_[query];
    if(!result.cached){
        auto status=vkGetFenceStatus(device_,fence_);if(status==VK_NOT_READY)return QueryRead::Pending;
        if(status!=VK_SUCCESS){error="Native query frame fence: "+std::to_string(int(status));return QueryRead::Failed;}
        std::uint64_t data[2]{};
        status=vkGetQueryPoolResults(device_,pool_,query,1,sizeof(data),data,sizeof(data),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
        if(status==VK_NOT_READY)return QueryRead::Pending;
        if(status!=VK_SUCCESS){error="Native occlusion query results: "+std::to_string(int(status));return QueryRead::Failed;}
        if(!data[1])return QueryRead::Pending;
        result.value=data[0];result.cached=true;
    }
    samples=result.value;generation=generation_;return QueryRead::Ready;
}
}
