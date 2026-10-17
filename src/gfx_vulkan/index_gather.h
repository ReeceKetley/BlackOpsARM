#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
namespace bo1::vulkan {
struct IndexSpan {const std::uint16_t* indices{};std::size_t count{};};
// Validate the complete gather before changing either output. The caller owns
// every CPU span through this synchronous copy; no GPU resource is published.
inline bool GatherIndices(const IndexSpan* spans,std::size_t count,std::size_t capacity,
    std::vector<std::uint16_t>& output,unsigned& vertexCount) {
    if(!spans||!count||!capacity)return false;
    std::size_t total=0;
    for(std::size_t i=0;i<count;++i) {
        if(!spans[i].indices||!spans[i].count||spans[i].count>capacity-total)return false;
        total+=spans[i].count;
    }
    if(total>output.max_size())return false;
    output.resize(total);std::size_t cursor=0;unsigned maximum=0;
    for(std::size_t i=0;i<count;++i) {
        std::memcpy(output.data()+cursor,spans[i].indices,spans[i].count*sizeof(std::uint16_t));cursor+=spans[i].count;
    }
    for(const auto index:output)if(index>maximum)maximum=index;
    vertexCount=maximum+1;return true;
}
}
