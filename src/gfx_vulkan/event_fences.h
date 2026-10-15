#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
namespace bo1::vulkan::event_fences {
struct Token;
bool Attach(VkDevice,VkQueue,std::string&);bool Empty();
bool Allocate(const void*,std::string&);
bool Insert(const void* alias,const void* poolOwner,std::string&);
bool Pending(const void* alias,bool&,std::string&);
bool WaitQueue(std::string&);bool Clear(std::string&);
struct Arena {std::vector<std::shared_ptr<Token>> tokens;bool Retire(std::string&);};
class RecordingScope {
public:
 explicit RecordingScope(Arena&);~RecordingScope();
 RecordingScope(const RecordingScope&)=delete;RecordingScope& operator=(const RecordingScope&)=delete;
 bool Submitted(std::string&);Arena& arena;std::vector<std::shared_ptr<Token>> markers;
private:RecordingScope* previous_;
};
}

#if defined(__ANDROID__)
void R_InitNativeEventQueries();
#endif
