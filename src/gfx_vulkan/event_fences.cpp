#include "event_fences.h"
#include "pointer_hash.h"
#include <mutex>
#include <unordered_map>
#include <thread>
namespace bo1::vulkan::event_fences {
struct Token {
 VkDevice device{};VkFence fence{};enum class Phase{Idle,Recorded,Submitted};Phase phase=Phase::Idle;
 ~Token(){if(fence)vkDestroyFence(device,fence,nullptr);}
};
namespace {
 std::mutex mutex;VkDevice device{};VkQueue queue{};std::thread::id queueOwner;
 std::unordered_map<const void*,std::shared_ptr<Token>,bo1::vulkan::PointerHash> pools,aliases;
 std::vector<std::weak_ptr<Token>> live;thread_local RecordingScope* current{};
 bool Check(VkResult result,const char* what,std::string& error){if(result==VK_SUCCESS)return true;error=std::string(what)+": "+std::to_string(int(result));return false;}
 bool Drained(){for(auto it=live.begin();it!=live.end();)if(it->expired())it=live.erase(it);else ++it;return pools.empty()&&aliases.empty()&&live.empty();}
 std::shared_ptr<Token> New(std::string& error) {
  auto token=std::make_shared<Token>();token->device=device;VkFenceCreateInfo create{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  if(!Check(vkCreateFence(device,&create,nullptr,&token->fence),"Native event fence allocation",error))return {};
  for(auto it=live.begin();it!=live.end();)if(it->expired())it=live.erase(it);else ++it;live.push_back(token);return token;
 }
 bool Signal(Token& token,std::string& error){if(!Check(vkQueueSubmit(queue,0,nullptr,token.fence),"Native event marker submission",error))return false;token.phase=Token::Phase::Submitted;return true;}
}
bool Empty(){std::lock_guard<std::mutex> lock(mutex);return Drained();}
bool Attach(VkDevice next,VkQueue nextQueue,std::string& error){std::lock_guard<std::mutex> lock(mutex);
 if(next==device&&nextQueue==queue)return true;if((!next)!=(!nextQueue)||!Drained()){error="Native event context incomplete or still owned";return false;}
 device=next;queue=nextQueue;queueOwner=std::this_thread::get_id();return true;
}
bool Allocate(const void* owner,std::string& error){std::lock_guard<std::mutex> lock(mutex);
 if(!device||!owner||pools.count(owner)){error="Invalid native event owner allocation";return false;}
 auto token=New(error);if(!token)return false;pools.emplace(owner,token);return true;
}
RecordingScope::RecordingScope(Arena& a):arena(a),previous_(current){current=this;}
RecordingScope::~RecordingScope(){current=previous_;}
bool RecordingScope::Submitted(std::string& error){std::lock_guard<std::mutex> lock(mutex);
 queueOwner=std::this_thread::get_id();for(auto& marker:markers)if(!Signal(*marker,error))return false;return true;
}
bool Insert(const void* alias,const void* owner,std::string& error){std::lock_guard<std::mutex> lock(mutex);
 auto found=pools.find(owner);if(!alias||found==pools.end()){error="Native event insertion needs an allocated pool owner";return false;}
 if(!current&&queueOwner!=std::this_thread::get_id()){error="Native event queue operation on a non-owner thread";return false;}
 auto token=found->second;
 if(token->phase!=Token::Phase::Idle){token=New(error);if(!token)return false;found->second=token;}
 token->phase=Token::Phase::Recorded;aliases[alias]=token;
 if(current){current->markers.push_back(token);current->arena.tokens.push_back(token);return true;}
 return Signal(*token,error);
}
bool Pending(const void* alias,bool& pending,std::string& error){std::lock_guard<std::mutex> lock(mutex);
 auto found=aliases.find(alias);if(found==aliases.end()){pending=false;return true;}
 auto& token=*found->second;if(token.phase!=Token::Phase::Submitted){pending=true;return true;}
 auto result=vkGetFenceStatus(device,token.fence);if(result==VK_NOT_READY){pending=true;return true;}
 if(!Check(result,"Native event completion",error))return false;pending=false;return true;
}
bool Arena::Retire(std::string& error){for(auto& token:tokens)if(token->phase==Token::Phase::Submitted)
 if(!Check(vkWaitForFences(token->device,1,&token->fence,VK_TRUE,1000000000ull),"Native event arena retirement",error))return false;
 tokens.clear();return true;}
bool WaitQueue(std::string& error){std::lock_guard<std::mutex> lock(mutex);
 if(!device||current||queueOwner!=std::this_thread::get_id()){error="Native GPU finish requires submitted work on its queue owner";return false;}
 return Check(vkQueueWaitIdle(queue),"Native GPU queue finish",error);
}
bool Clear(std::string& error){std::lock_guard<std::mutex> lock(mutex);
 if(current){error="Native event shutdown requires frame recording to finish";return false;}
 for(auto& item:aliases)if(item.second->phase==Token::Phase::Submitted)
  if(!Check(vkWaitForFences(device,1,&item.second->fence,VK_TRUE,1000000000ull),"Native event shutdown wait",error))return false;
 aliases.clear();pools.clear();return true;
}
}
