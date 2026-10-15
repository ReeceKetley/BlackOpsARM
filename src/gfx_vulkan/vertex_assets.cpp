#include "vertex_assets.h"
#include "pointer_hash.h"
#include "shader_assets.h"
#include "gfx_d3d/r_material.h"
#include "database/db_material_graph_types.h"
#include <mutex>
#include <unordered_map>
#include <cctype>
namespace bo1::vulkan::vertex_assets {
namespace {
std::mutex mutex;
std::unordered_map<const MaterialVertexDeclaration*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> owners;
bool Same(const std::string& a,const char* b) {unsigned i=0;for(;i<a.size()&&b[i];++i)if(std::toupper(static_cast<unsigned char>(a[i]))!=b[i])return false;return i==a.size()&&!b[i];}
const char* names[]={"POSITION","BLENDWEIGHT","BLENDINDICES","NORMAL","PSIZE","TEXCOORD","TANGENT","BINORMAL","TESSFACTOR","POSITIONT","COLOR","FOG","DEPTH","SAMPLE"};
const VkFormat formats[]={VK_FORMAT_R32_SFLOAT,VK_FORMAT_R32G32_SFLOAT,VK_FORMAT_R32G32B32_SFLOAT,VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_B8G8R8A8_UNORM,VK_FORMAT_R8G8B8A8_USCALED,VK_FORMAT_R16G16_SSCALED,VK_FORMAT_R16G16B16A16_SSCALED,
    VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_R16G16_SNORM,VK_FORMAT_R16G16B16A16_SNORM,VK_FORMAT_R16G16_UNORM,VK_FORMAT_R16G16B16A16_UNORM,
    VK_FORMAT_UNDEFINED,VK_FORMAT_UNDEFINED,VK_FORMAT_R16G16_SFLOAT,VK_FORMAT_R16G16B16A16_SFLOAT};
const unsigned sizes[]={4,8,12,16,4,4,4,8,4,4,8,4,8,4,4,4,8};
}
bool Create(MaterialVertexDeclaration* owner,const stream_source_info_t* const* rows,const stream_dest_info_t* destinations,std::string& error) {
    error.clear();if(!owner||!rows||!destinations||owner->streamCount>16||!shader_assets::Active()) {error="Invalid native vertex declaration context";return false;}
    for(auto legacy:owner->routing.decl)if(legacy) {error="Cannot replace live D3D9 declaration";return false;}
    auto result=std::make_shared<Record>();
    for(unsigned v=0;v<18;++v) {
        if(!rows[v]) {error="Missing canonical source table";return false;}
        auto& variant=result->variants[v];variant.available=true;
        for(unsigned i=0;i<owner->streamCount;++i) {
            const auto route=owner->routing.data[i];
            if(route.source>=10||route.dest>=20) {error="Vertex routing index out of bounds";return false;}
            const auto& source=rows[v][route.source];const auto& dest=destinations[route.dest];
            if(source.Stream==255) {variant.available=false;variant.elements.clear();break;}
            if(source.Type>=17||dest.Usage>=14) {error="Unsupported canonical vertex element";return false;}
            for(const auto& existing:variant.elements)if(existing.usage==dest.Usage&&existing.usageIndex==dest.UsageIndex) {error="Duplicate vertex semantic";return false;}
            variant.elements.push_back({source.Stream,source.Offset,source.Type,dest.Usage,dest.UsageIndex});
        }
    }
    std::lock_guard<std::mutex> lock(mutex);if(!shader_assets::Active()) {error="Native vertex backend changed during creation";return false;}
    owners[owner]=std::move(result);return true;
}
std::shared_ptr<const Record> Acquire(const MaterialVertexDeclaration* owner) {std::lock_guard<std::mutex> lock(mutex);const auto i=owners.find(owner);return i==owners.end()?nullptr:i->second;}
void Release(const MaterialVertexDeclaration* owner) {std::lock_guard<std::mutex> lock(mutex);owners.erase(owner);}
bool Empty(){std::lock_guard<std::mutex> lock(mutex);return owners.empty();}
namespace {
// Format capabilities are immutable per physical device; draw resolution only
// needs the buffer feature bits. Cached per render thread, keyed by device.
VkFormatFeatureFlags BufferFeatures(VkPhysicalDevice physical,VkFormat format) {
    thread_local VkPhysicalDevice cachedDevice{};thread_local std::unordered_map<int,VkFormatFeatureFlags> cached;
    if(cachedDevice!=physical){cached.clear();cachedDevice=physical;}
    const auto it=cached.find(int(format));if(it!=cached.end())return it->second;
    VkFormatProperties properties{};vkGetPhysicalDeviceFormatProperties(physical,format,&properties);
    cached.emplace(int(format),properties.bufferFeatures);return properties.bufferFeatures;
}
}
bool Resolve(const Record& record,unsigned v,const ShaderTranslation& shader,VkPhysicalDevice physical,
    const std::vector<VkVertexInputBindingDescription>& bindings,std::vector<VkVertexInputAttributeDescription>& attributes,std::string& error,unsigned* vertexUbyteMask) {
    attributes.clear();error.clear();if(vertexUbyteMask)*vertexUbyteMask=0;unsigned ubyteMask=0;
    if(v>=18||!record.variants[v].available||shader.stage!=ShaderStage::Vertex||!physical) {error="Unavailable vertex variant or shader/device";return false;}
    std::vector<VkVertexInputAttributeDescription> result;
    for(const auto& input:shader.inputs) {
        if(input.systemValue||!input.usedMask)continue;
        const Element* found{};
        for(const auto& e:record.variants[v].elements)if(Same(input.semantic,names[e.usage])&&input.semanticIndex==e.usageIndex) {found=&e;break;}
        if(!found) {error="Missing routed vertex semantic "+input.semantic;return false;}
        auto format=formats[found->type];
        if(format==VK_FORMAT_UNDEFINED) {error="Packed UDEC3/DEC3N requires native shader decoding";return false;}
        const VkVertexInputBindingDescription* binding{};
        for(const auto& b:bindings)if(b.binding==found->stream) {if(binding) {error="Duplicate vertex binding";return false;}binding=&b;}
        if(!binding||(binding->stride&&found->offset+sizes[found->type]>binding->stride)) {error="Missing/short vertex stream";return false;}
        VkFormatFeatureFlags features=BufferFeatures(physical,format);
        if(!(features&VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)&&found->type==5&&vertexUbyteMask&&input.registerIndex<16) {
            format=VK_FORMAT_R8G8B8A8_UNORM;features=BufferFeatures(physical,format);
            if(features&VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)ubyteMask|=1u<<input.registerIndex;
        }
        if(!(features&VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)) {error="Device cannot fetch routed vertex format="+std::to_string(unsigned(format))+" type="+std::to_string(found->type)
            +" variant="+std::to_string(v)+" semantic="+input.semantic+std::to_string(input.semanticIndex)
            +" register="+std::to_string(input.registerIndex)+" stream="+std::to_string(found->stream);return false;}
        result.push_back({input.registerIndex,found->stream,format,found->offset});
    }
    attributes=std::move(result);if(vertexUbyteMask)*vertexUbyteMask=ubyteMask;return true;
}
}
