#pragma once
#include "resources.h"
#include <memory>
#include <cstdint>
#include <initializer_list>
namespace bo1::vulkan::buffer_assets {
enum class Kind { Vertex,Index16 };
struct Record {std::uint64_t serial{};Kind kind{};std::size_t payloadBytes{};bool dynamic{};std::shared_ptr<Buffer> gpu;};
// Stable owner SLOT addresses, never fabricated COM buffer pointers. Retained
// records must survive the last submitted reader; device changes require drain.
struct OwnerCopy {const void* source{};const void* destination{};};
// Mirror a canonical asset copy; absent source remains absent, never fabricated.
bool CloneOwners(std::initializer_list<OwnerCopy>,std::string&);
bool Attach(VkPhysicalDevice,VkDevice,std::string&);
bool Empty();
bool Load(const void* owner,const void* bytes,std::size_t payload,std::size_t allocation,Kind,std::string&);
bool Reserve(const void* owner,std::size_t allocation,Kind,std::string&);
void* Begin(const void* owner,std::size_t allocation,Kind,std::string&);
bool Finish(const void* owner,Kind,std::string&);
void* MapDiscard(const void* owner,std::string&,bool preserve=false);
bool UpdateRange(const void* owner,std::size_t offset,const void* bytes,std::size_t payload,std::string&);
// Append only to previously unused bytes. Offset zero discards the old contents;
// a backing retained by any command/frame is then replaced, never overwritten.
bool Write(const void* owner,std::size_t offset,const void* bytes,std::size_t payload,std::string&);
std::shared_ptr<const Record> Acquire(const void* owner);
void Release(const void* owner);
}
