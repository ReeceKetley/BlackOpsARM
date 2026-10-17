#pragma once
#include "image_assets.h"
#include <algorithm>
#include <cstring>
namespace bo1::vulkan::model_lighting_regions {
inline bool Valid(const image_assets::Record& r) {
    const auto& d=r.description;
    return r.sourceFormat==21&&r.bytesPerTexel==4&&!r.sourceBlockBytes&&!r.nativeBlocks
        &&d.format==VK_FORMAT_B8G8R8A8_UNORM&&d.type==VK_IMAGE_TYPE_3D&&d.viewType==VK_IMAGE_VIEW_TYPE_3D
        &&d.extent.width==256&&d.extent.depth==4&&d.extent.height&&d.extent.height<=16384&&!(d.extent.height%4)
        &&d.mipLevels==1&&d.arrayLayers==1&&d.aspects==VK_IMAGE_ASPECT_COLOR_BIT
        &&r.uploads.size()==1&&!r.uploads[0].mip&&!r.uploads[0].layer&&r.uploads[0].bytes
        &&r.uploads[0].bytes->size()==std::size_t(256)*d.extent.height*16;
}
inline std::size_t Offset(unsigned tile,unsigned height,unsigned z,unsigned y) {
    return std::size_t(z)*height*1024+std::size_t(4*(tile>>6)+y)*1024+16*(tile&63);
}
inline bool SameTile(const void* a,const void* b,unsigned tile,unsigned height) {
    for(unsigned z=0;z<4;++z)for(unsigned y=0;y<4;++y) {
        const auto offset=Offset(tile,height,z,y);
        if(std::memcmp(static_cast<const unsigned char*>(a)+offset,static_cast<const unsigned char*>(b)+offset,16))return false;
    }
    return true;
}
inline void PackTile(const void* source,unsigned tile,unsigned height,void* destination) {
    auto* out=static_cast<unsigned char*>(destination);
    for(unsigned z=0;z<4;++z)for(unsigned y=0;y<4;++y)
        std::memcpy(out+(z*4+y)*16,static_cast<const unsigned char*>(source)+Offset(tile,height,z,y),16);
}
inline bool Changed(const image_assets::Record& previous,const void* source,const std::vector<unsigned>& tiles,
    std::vector<unsigned>& changed,std::size_t& uniqueCount,std::string& error) {
    if(!source||!Valid(previous)){error="Invalid model-lighting region snapshot";return false;}
    auto unique=tiles;std::sort(unique.begin(),unique.end());unique.erase(std::unique(unique.begin(),unique.end()),unique.end());
    for(auto tile:unique)if(tile>=16u*previous.description.extent.height){error="Model-lighting tile exceeds atlas";return false;}
    std::vector<unsigned> result;
    for(auto tile:unique)if(!SameTile(source,previous.uploads[0].bytes->data(),tile,previous.description.extent.height))result.push_back(tile);
    changed=std::move(result);uniqueCount=unique.size();return true;
}
inline void Pack(const image_assets::Record& record,const std::vector<unsigned>& tiles,VkDeviceSize offset,
    std::vector<unsigned char>& bytes,std::vector<VkBufferImageCopy>& regions) {
    bytes.resize(tiles.size()*256);regions.clear();regions.reserve(tiles.size());
    for(std::size_t i=0;i<tiles.size();++i) {
        PackTile(record.uploads[0].bytes->data(),tiles[i],record.description.extent.height,bytes.data()+i*256);
        VkBufferImageCopy c{};c.bufferOffset=offset+i*256;c.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        c.imageOffset={int(4*(tiles[i]&63)),int(4*(tiles[i]>>6)),0};c.imageExtent={4,4,4};regions.push_back(c);
    }
}
struct Box {unsigned x{},y{},width{},height{};std::size_t bytes{};};
// All tiles cover z=0..3. One rectangle includes unchanged holes from the
// newest complete snapshot. Large/invalid boxes use the existing full copy.
inline bool PlanBox(const image_assets::Record& record,const std::vector<unsigned>& tiles,Box& box) {
    box={};if(!Valid(record)||tiles.empty())return false;
    unsigned left=63,right=0,top=record.description.extent.height/4,bottom=0;
    for(auto tile:tiles) {
        if(tile>=16u*record.description.extent.height)return false;
        left=std::min(left,tile&63);right=std::max(right,tile&63);
        top=std::min(top,tile>>6);bottom=std::max(bottom,tile>>6);
    }
    Box candidate{left*4,top*4,(right-left+1)*4,(bottom-top+1)*4,0};
    candidate.bytes=std::size_t(candidate.width)*candidate.height*4*4;
    if(candidate.bytes>record.uploads[0].bytes->size()/2)return false;
    box=candidate;return true;
}
inline void PackBox(const image_assets::Record& record,const Box& box,VkDeviceSize offset,
    std::vector<unsigned char>& bytes,std::vector<VkBufferImageCopy>& regions) {
    bytes.resize(box.bytes);regions.clear();
    const auto* source=record.uploads[0].bytes->data();const std::size_t rowBytes=std::size_t(box.width)*4;
    for(unsigned z=0;z<4;++z)for(unsigned y=0;y<box.height;++y) {
        const auto from=(std::size_t(z)*record.description.extent.height+box.y+y)*1024+box.x*4;
        const auto to=(std::size_t(z)*box.height+y)*rowBytes;
        std::memcpy(bytes.data()+to,source+from,rowBytes);
    }
    VkBufferImageCopy c{};c.bufferOffset=offset;c.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
    c.imageOffset={int(box.x),int(box.y),0};c.imageExtent={box.width,box.height,4};regions.push_back(c);
}
// Queue coalescing never replaces a later snapshot with a restored older one.
// A broken/missing ancestry falls back to the latest complete CPU snapshot.
struct Pending {
    std::shared_ptr<const image_assets::Record> record;
    bool partial{};std::uint64_t base{};std::vector<unsigned> tiles;
    explicit Pending(std::shared_ptr<const image_assets::Record> r):record(std::move(r)),
        partial(record->modelLightingRegions),base(record->regionBaseRevision),tiles(record->changedTiles){}
};
inline Pending Merge(Pending a,Pending b) {
    if(a.record==b.record){if(!a.partial)return a;if(!b.partial)return b;}
    if(a.record->revision>b.record->revision)std::swap(a,b);
    if(a.record->texture!=b.record->texture||!b.partial||!a.partial||b.base!=a.record->revision) {
        b.partial=false;b.tiles.clear();return b;
    }
    b.base=a.base;b.tiles.insert(b.tiles.end(),a.tiles.begin(),a.tiles.end());
    std::sort(b.tiles.begin(),b.tiles.end());b.tiles.erase(std::unique(b.tiles.begin(),b.tiles.end()),b.tiles.end());return b;
}
inline bool CanPartial(const Pending& queued,const std::shared_ptr<Texture2D>& orderedTexture,
    std::uint64_t orderedRevision,bool defined) {
    if(!orderedTexture||!queued.partial||!Valid(*queued.record)||queued.tiles.empty()||queued.record->texture!=orderedTexture
        ||queued.base!=orderedRevision||!defined)return false;
    for(auto tile:queued.tiles)if(tile>=16u*queued.record->description.extent.height)return false;
    return true;
}

}
