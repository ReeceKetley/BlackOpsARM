#pragma once
#include "shader_translate.h"
#include <vulkan/vulkan.h>
#include <array>
#include <memory>
struct MaterialVertexDeclaration;struct stream_source_info_t;struct stream_dest_info_t;
namespace bo1::vulkan::vertex_assets {
struct Element {unsigned stream,offset,type,usage,usageIndex;};
struct Variant {bool available{};std::vector<Element> elements;};
struct Record {std::array<Variant,18> variants;};
bool Create(MaterialVertexDeclaration*,const stream_source_info_t* const* rows,
    const stream_dest_info_t* destinations,std::string& error);
std::shared_ptr<const Record> Acquire(const MaterialVertexDeclaration*);
void Release(const MaterialVertexDeclaration*);
bool Empty();
bool Resolve(const Record&,unsigned variant,const ShaderTranslation& vertex,
    VkPhysicalDevice,const std::vector<VkVertexInputBindingDescription>& bindings,
    std::vector<VkVertexInputAttributeDescription>& attributes,std::string& error,unsigned* vertexUbyteMask=nullptr);
}
