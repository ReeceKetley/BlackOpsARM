#include "model_lighting_regions.h"
#include <cstdio>
#include <stdexcept>
using namespace bo1::vulkan;
namespace {void Check(bool value,const char* text){if(!value)throw std::runtime_error(text);}}
bool NativeModelLightingRegionChecks() {
    using namespace model_lighting_regions;
    try {
        std::string error;constexpr unsigned height=8;std::vector<unsigned char> source(256*height*16,0);
        auto a=std::make_shared<image_assets::Record>();a->sourceFormat=21;a->bytesPerTexel=4;a->revision=1;
        a->description.format=VK_FORMAT_B8G8R8A8_UNORM;a->description.type=VK_IMAGE_TYPE_3D;a->description.viewType=VK_IMAGE_VIEW_TYPE_3D;
        a->description.extent={256,height,4};a->uploads={{0,0,UploadBytes::Copy(source.data(),source.size(),error)}};
        // CPU-only identity token: never dereferenced as a texture. The GPU
        // fixture below separately uses an actual allocated Texture2D owner.
        auto identity=std::make_shared<unsigned char>();
        a->texture=std::shared_ptr<Texture2D>(identity,reinterpret_cast<Texture2D*>(identity.get()));
        Check(Valid(*a),"actual atlas description rejected");
        std::vector<unsigned> changed;std::size_t count{};
        Check(Changed(*a,source.data(),{0,127,0},changed,count,error)&&changed.empty()&&count==2,"unchanged/duplicate detection");
        source[Offset(127,height,3,3)+15]=77;
        Check(Changed(*a,source.data(),{127,127,0},changed,count,error)&&changed==std::vector<unsigned>{127}&&count==2,"last tile bounds");
        Check(!Changed(*a,source.data(),{128},changed,count,error)&&changed==std::vector<unsigned>{127},"invalid index must fail transactionally");
        auto b=std::make_shared<image_assets::Record>(*a);b->revision=2;b->regionBaseRevision=1;b->modelLightingRegions=true;b->changedTiles={127};
        b->uploads[0].bytes=UploadBytes::Copy(source.data(),source.size(),error);
        source.assign(source.size(),255);
        Check(b->uploads[0].bytes->data()[Offset(127,height,3,3)+15]==77&&a->uploads[0].bytes->data()[0]==0,"immutable publication/source overwrite");
        std::vector<unsigned char> packed;std::vector<VkBufferImageCopy> copies;Pack(*b,{127},16,packed,copies);
        Check(packed.size()==256&&packed.back()==77&&copies.size()==1&&copies[0].bufferOffset==16
            &&copies[0].imageOffset.x==252&&copies[0].imageOffset.y==4&&copies[0].imageExtent.depth==4,"actual region packing");
        auto c=std::make_shared<image_assets::Record>(*b);c->revision=3;c->regionBaseRevision=2;c->changedTiles={0};
        auto merged=Merge(Pending(b),Pending(c));
        Check(merged.record==c&&merged.base==1&&merged.tiles==std::vector<unsigned>({0,127})&&merged.partial,"pending union lost older tiles");
        auto restored=Merge(Pending(c),Pending(b));
        Check(restored.record==c&&restored.base==1&&restored.tiles==merged.tiles,"failed flush restored older over newer");
        Check(CanPartial(merged,a->texture,1,true)&&!CanPartial(merged,a->texture,2,true)&&!CanPartial(merged,a->texture,1,false)&&!CanPartial(merged,{},1,true),"ancestry/initialization gate");
        auto full=Merge(Pending(a),Pending(c));Check(!full.partial&&full.record==c,"full initialization supersession");
        auto gap=std::make_shared<image_assets::Record>(*c);gap->revision=5;gap->regionBaseRevision=4;
        Check(!Merge(Pending(b),Pending(gap)).partial,"missing ancestry requires full recovery");
        auto invalid=*a;invalid.description.format=VK_FORMAT_R8G8B8A8_UNORM;Check(!Valid(invalid),"format mismatch accepted");
        invalid=*a;invalid.description.extent.depth=3;Check(!Valid(invalid),"invalid atlas shape accepted");
        Box box;
        Check(PlanBox(*a,{0,2},box)&&box.bytes==768&&box.width==12&&box.height==4,"clustered box with hole");
        Check(PlanBox(*a,{0,63},box)&&box.bytes==source.size()/2,"half-atlas box boundary rejected");
        Check(!PlanBox(*a,{63,64},box)&&!box.bytes,"over-half wrapped box must use full copy");
        Check(!PlanBox(*a,{128},box)&&!PlanBox(*a,{},box),"invalid/empty box accepted");
        std::vector<unsigned char> pattern(source.size());for(std::size_t i=0;i<pattern.size();++i)pattern[i]=static_cast<unsigned char>((i*29+i/1024)%251);
        auto patterned=*a;patterned.uploads[0].bytes=UploadBytes::Copy(pattern.data(),pattern.size(),error);
        Check(PlanBox(patterned,{1,3,66},box),"patterned rectangle plan rejected");PackBox(patterned,box,32,packed,copies);
        Check(copies.size()==1&&!copies[0].bufferRowLength&&!copies[0].bufferImageHeight&&copies[0].bufferOffset==32,"box must be tightly packed 3D copy");
        for(unsigned z=0;z<4;++z)for(unsigned y=0;y<box.height;++y)for(unsigned x=0;x<box.width*4;++x) {
            const auto expected=(std::size_t(z)*height+box.y+y)*1024+box.x*4+x;
            Check(packed[(std::size_t(z)*box.height+y)*box.width*4+x]==pattern[expected],"box row/slice/unchanged-hole packing mismatch");
        }
        std::printf("native_model_lighting_regions=PASS unchanged=1 duplicates=1 bounds=1 immutable=1 union=1 restore_newer=1 full_fallback=1\n");return true;
    } catch(const std::exception& e){std::fprintf(stderr,"native_model_lighting_regions=FAIL %s\n",e.what());return false;}
}
#ifdef BO1_REGION_HOST_TEST
int main(){return NativeModelLightingRegionChecks()?0:1;}
#endif
