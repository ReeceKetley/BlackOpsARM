"""Host checks of production near-shadow stream decoder, geometry admission and state."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[3]
out=root/'build-android-audit'/'near-shadow-cache-test';out.mkdir(exist_ok=True)
source=(root/'src/gfx_d3d/r_draw_sunshadow.cpp').read_bytes().decode('cp1252')
a=source.index('bool R_NearGeometry(');b=source.index('void R_PrepareNearCache(',a)
geometry=source[a:b]
a=source.index("void R_DrawNearKind(");b=source.index("// kind: -1",a)
draw_prefix=source[a:b]
prefix=r'''
#include "gfx_vulkan/near_shadow_cache.h"
#include <cstdio>
#include <limits>
using namespace bo1::vulkan::sun_bmodel_cache;
namespace near=bo1::vulkan::near_shadow_cache;
unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL line %u\n",unsigned(__LINE__));return 1;}}while(0)
struct Tri {unsigned firstVertex{},vertexCount{},baseIndex{},triCount{},vertexLayerData{},stream2ByteOffset{};};
struct Surface {Tri tris;};
struct XSurface {unsigned vertCount{},triCount{},baseTriIndex{},baseVertIndex{},flags{};void* vb0{};void* indexBuffer{};};
struct Lod {unsigned surfIndex{},numsurfs{};};
struct Model {XSurface* surfs{};unsigned numsurfs{};Lod lodInfo[4];};
struct Placement {float origin[3]{},axis[3][3]{},scale{1};};
struct Instance {Placement placement;Model* model{};unsigned smodelCacheIndex[4]{};};
struct World {unsigned surfaceCount{};struct {Surface* surfaces{};unsigned smodelCount{};Instance* smodelDrawInsts{};} dpvs;struct {int indexCount{};unsigned short* indices{};} draw;};
struct {World* world;} rgp;
struct {void* smodelCacheVb;struct {unsigned short* indices;} smodelCache;} gfxBuf;
struct Dyn {unsigned maxDynSModelIndexInMap{};};
struct View {Dyn* dynSModelState{};};
struct GfxDrawSurf {unsigned identity;};
struct GfxDrawSurfListInfo {View* viewInfo;GfxDrawSurf* drawSurfs{};unsigned drawSurfCount{};};
struct GfxCmdBufContext {};
std::vector<unsigned char> s_nearEligible;unsigned s_nearStaticDraws{},s_nearDynamicDraws{};
std::vector<unsigned> emitted;unsigned actualDraws{};
namespace bo1::vulkan::material_dispatch {struct Profile {unsigned draws;};Profile ReadProfile(){return {actualDraws};}}
void R_DrawSurfs(GfxCmdBufContext,int,const GfxDrawSurfListInfo* list){for(unsigned i=0;i<list->drawSurfCount;++i){emitted.push_back(list->drawSurfs[i].identity);++actualDraws;}}

struct Leaf {struct {unsigned baseVertIndex{},lodIndex{},smodelIndex{};} cachedSurf;} leaves[16384];
unsigned leafReads{};Leaf* SMC_GetLeaf(unsigned i){++leafReads;return &leaves[i-1];}
XSurface* decodedSurface{};
#define BO1_NARROW_DECODE(T,h) (decodedSurface)
bool ownersAvailable=true;unsigned long long ownerSerial=1;
bool R_NearOwner(Values& out,const void* owner){if(!ownersAvailable)return false;out.push_back(reinterpret_cast<std::uintptr_t>(owner));out.push_back(ownerSerial);return true;}
'''
body=r'''
int main(){
 GfxDrawSurf drawEntries[]={{1},{2},{3},{4},{5}};GfxDrawSurfListInfo drawList{nullptr,drawEntries,5};
 s_nearEligible={1,1,1,0,0};R_DrawNearKind({},&drawList,true);R_DrawNearKind({},&drawList,false);
 CHECK((emitted==std::vector<unsigned>{1,2,3,4,5}));CHECK(s_nearStaticDraws==3&&s_nearDynamicDraws==2);
 emitted.clear();R_DrawNearKind({},&drawList,false);CHECK((emitted==std::vector<unsigned>{4,5}));
 emitted.clear();s_nearEligible.clear();R_DrawNearKind({},&drawList,false);CHECK((emitted==std::vector<unsigned>{1,2,3,4,5}));
 CHECK(near::DepthPassSafe(5));CHECK(near::DepthPassSafe(13));
 for(unsigned word:{0u,1u,2u,4u,8u,9u,12u,15u,0x45u,0xcdu})CHECK(!near::DepthPassSafe(word));
 unsigned bsp[]={3,0x00020001,0xabcd0003,0};Values record;
 auto emit=[](unsigned type,unsigned surface,unsigned id,Values& values){values.push_back(type);values.push_back(surface);values.push_back(id);return true;};
 CHECK(near::Decode(bsp,4,0,0,record,emit));auto original=record;
 bsp[2]=0xffff0003;record.clear();CHECK(near::Decode(bsp,4,0,0,record,emit)&&record==original);
 for(unsigned size=0;size<4;++size){record.clear();CHECK(!near::Decode(bsp,size,0,0,record,emit));}
 CHECK(!near::Decode(bsp,4,4,0,record,emit));
 for(unsigned type: {1,3,5,6,7,8,9,10})CHECK(!near::Decode(bsp,4,0,type,record,emit));
 unsigned huge[]={0xffffffff,1,0};CHECK(!near::Decode(huge,3,0,0,record,emit));
 unsigned rigid[]={0x10000002,7,0x00020001,0};record.clear();CHECK(near::Decode(rigid,4,0,2,record,emit));CHECK(record[0]==0x10000002);
 unsigned cached[]={2,7,0x00020001,0};record.clear();CHECK(near::Decode(cached,4,0,4,record,emit));
 cached[1]=0;CHECK(!near::Decode(cached,4,0,4,record,emit));cached[1]=7;
 CHECK(!near::Decode(cached,4,0,4,record,[](unsigned,unsigned,unsigned,Values&){return false;}));
 unsigned short rawIndices[1024]{};Values exact;CHECK(near::AppendIndices(exact,rawIndices,1024,1021,3));
 CHECK(!near::AppendIndices(exact,rawIndices,1024,1022,3));CHECK(!near::AppendIndices(exact,rawIndices,1024,~0ull,1));
 auto unchanged=exact;rawIndices[1023]=7;exact.clear();CHECK(near::AppendIndices(exact,rawIndices,1024,1021,3)&&exact!=unchanged);
 Surface surfaces[2]{};Instance instances[2]{};World world{2,{surfaces,2,instances}};rgp.world=&world;world.draw={1024,rawIndices};gfxBuf.smodelCache.indices=rawIndices;
 XSurface xs[2]{};xs[0].triCount=1;Model model{xs,2,{{0,2}}};instances[1].model=&model;decodedSurface=xs;
 Dyn dyn{};View view{&dyn};GfxDrawSurfListInfo list{&view};
 record.clear();CHECK(R_NearGeometry(0,0,1,record,list));CHECK(!R_NearGeometry(0,0,2,record,list));
 record.clear();CHECK(R_NearGeometry(2,7,1,record,list));original=record;
 instances[1].placement.origin[0]=5;record.clear();CHECK(R_NearGeometry(2,7,1,record,list)&&record!=original);instances[1].placement.origin[0]=0;
 instances[1].placement.scale=std::numeric_limits<float>::quiet_NaN();CHECK(!R_NearGeometry(2,7,1,record,list));instances[1].placement.scale=1;
 dyn.maxDynSModelIndexInMap=2;CHECK(!R_NearGeometry(2,7,1,record,list));dyn.maxDynSModelIndexInMap=0;
 decodedSurface=reinterpret_cast<XSurface*>(reinterpret_cast<char*>(xs)+1);CHECK(!R_NearGeometry(2,7,1,record,list));
 decodedSurface=xs+2;CHECK(!R_NearGeometry(2,7,1,record,list));decodedSurface=xs;
 ownersAvailable=false;CHECK(!R_NearGeometry(2,7,1,record,list));ownersAvailable=true;
 CHECK(!R_NearGeometry(4,7,0,record,list));CHECK(!R_NearGeometry(4,7,16385,record,list));CHECK(leafReads==0);
 leaves[0].cachedSurf={50,0,1};instances[1].smodelCacheIndex[0]=1;
 record.clear();CHECK(R_NearGeometry(4,7,1,record,list));original=record;
 leaves[0].cachedSurf.baseVertIndex=51;record.clear();CHECK(R_NearGeometry(4,7,1,record,list)&&record!=original);leaves[0].cachedSurf.baseVertIndex=50;
 instances[1].smodelCacheIndex[0]=2;CHECK(!R_NearGeometry(4,7,1,record,list));instances[1].smodelCacheIndex[0]=1;
 leaves[0].cachedSurf.lodIndex=4;CHECK(!R_NearGeometry(4,7,1,record,list));leaves[0].cachedSurf.lodIndex=0;
 leaves[0].cachedSurf.smodelIndex=2;CHECK(!R_NearGeometry(4,7,1,record,list));leaves[0].cachedSurf.smodelIndex=1;
 model.lodInfo[0]={1,1};CHECK(!R_NearGeometry(4,7,1,record,list));model.lodInfo[0]={0,2};
 rawIndices[200]=5;record.clear();CHECK(R_NearGeometry(4,7,1,record,list)&&record!=original);rawIndices[200]=0;
 ++ownerSerial;record.clear();CHECK(R_NearGeometry(4,7,1,record,list)&&record!=original);
 near::State state;Values base{1,2,3};auto d=state.Update(base,{record});CHECK(!d.ready&&d.changed);
 auto stable=state.Update(base,{record});CHECK(stable.ready&&stable.key==d.key);
 CHECK(state.Update(base,{record}).ready);
 for(unsigned i=0;i<20;++i){auto moved=base;moved[1]+=i+1;CHECK(!state.Update(moved,{record}).ready);}
 CHECK(!state.Update(base,{record}).ready);stable=state.Update(base,{record});CHECK(stable.ready);
 auto changed=record;changed[0]++;CHECK(!state.Update(base,{changed}).ready);CHECK(state.Update(base,{changed}).ready);
 CHECK(!state.Update(base,{record,changed}).ready);stable=state.Update(base,{changed,record});CHECK(!stable.ready);
 CHECK(!state.Update(base,{record,changed}).ready);CHECK(state.Update(base,{record,changed}).ready);CHECK(!state.Update(base,{record}).ready);
 auto beforeReset=state.Update(base,{record}).key;state.Reset();d=state.Update(base,{record});CHECK(!d.ready&&d.key!=beforeReset);
 CHECK(!state.Update(base,{}).ready);CHECK(!state.Update(base,{}).ready);
 printf("PASS %u near stream/geometry/state checks\n",checks);
}
'''
fixture=out/'fixture.cpp';fixture.write_text(prefix+geometry+draw_prefix+body,encoding='ascii')
exe=out/'fixture.exe'
subprocess.run([r'C:\Program Files\LLVM\bin\clang++.exe','-std=c++17','-O2','-I',str(root/'src'),str(fixture),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
