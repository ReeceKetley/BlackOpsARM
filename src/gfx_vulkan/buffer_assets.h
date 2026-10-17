#pragma once
#include "resources.h"
#include <memory>
#include <cstdint>
#include <initializer_list>
namespace bo1::vulkan::buffer_assets {
enum class Kind { Vertex,Index16 };
// offset: where the payload starts in gpu (non-zero only for packed records,
// which share one immutable buffer; bind and index relative to it).
struct Record {std::uint64_t serial{};Kind kind{};std::size_t payloadBytes{};bool dynamic{};std::shared_ptr<Buffer> gpu;VkDeviceSize offset{};};
// Stable owner SLOT addresses, never fabricated COM buffer pointers. Retained
// records must survive the last submitted reader; device changes require drain.
struct OwnerCopy {const void* source{};const void* destination{};};
// Mirror a canonical asset copy; absent source remains absent, never fabricated.
bool CloneOwners(std::initializer_list<OwnerCopy>,std::string&);
bool Attach(VkPhysicalDevice,VkDevice,std::string&);
bool Empty();
bool Load(const void* owner,const void* bytes,std::size_t payload,std::size_t allocation,Kind,std::string&);
// As Load, for small immutable buffers: packs them into shared 16 MB buffers
// so thousands of model surfaces stay within the device allocation-count limit.
bool LoadPacked(const void* owner,const void* bytes,std::size_t payload,std::size_t allocation,Kind,std::string&);
bool Reserve(const void* owner,std::size_t allocation,Kind,std::string&);
void* Begin(const void* owner,std::size_t allocation,Kind,std::string&);
bool Finish(const void* owner,Kind,std::string&);
void* MapDiscard(const void* owner,std::string&,bool preserve=false);
// noOverwrite: D3DLOCK_NOOVERWRITE contract - the caller guarantees no
// submitted frame still reads [offset,offset+payload), so the bytes are
// written in place instead of copying the whole retained backing first.
bool UpdateRange(const void* owner,std::size_t offset,const void* bytes,std::size_t payload,std::string&,bool noOverwrite=false);
// Append only to previously unused bytes. Offset zero discards the old contents;
// a backing retained by any command/frame is then replaced, never overwritten.
bool Write(const void* owner,std::size_t offset,const void* bytes,std::size_t payload,std::string&);
std::shared_ptr<const Record> Acquire(const void* owner);
void Release(const void* owner);
}
