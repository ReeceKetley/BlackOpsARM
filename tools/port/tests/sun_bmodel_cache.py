"""Host regression for production brush serialization, material eligibility and cache state."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[3]
out=root/"build-android-audit"/"sun-bmodel-cache-test";out.mkdir(exist_ok=True)
s=(root/"src/gfx_d3d/r_draw_sunshadow.cpp").read_bytes().decode("cp1252").replace("\r\n","\n")
a=s.index("bool R_BrushMaterialSnapshot(");b=s.index("const char* R_BrushBaseField(",a)
material=s[a:b]
c=s.index("void R_PrepareSunBrushCache(");d=s.index("    const auto* frame=data->data;",c)
off_path=s[c:d]+"    ++snapshotBuilds;\n}\n"
prefix=r'''
#include "gfx_vulkan/sun_bmodel_cache.h"
#include "gfx_vulkan/sun_shadow_quality.h"
#include "gfx_vulkan/near_shadow_cache.h"
#include <cstdio>
#include <limits>
#include <unordered_map>
#include <array>
using namespace bo1::vulkan::sun_bmodel_cache;
unsigned shaderAcquires;
struct Shader { unsigned long long serial; };
namespace bo1::vulkan::shader_assets { Shader* Acquire(Shader* p){++shaderAcquires;return p;} }
struct DeclElement {unsigned stream{},offset{},type{},usage{},usageIndex{};};
struct DeclVariant {bool available=true;std::vector<DeclElement> elements{{}};};
struct Decl {std::array<DeclVariant,18> variants;};Decl vertexDecl;bool declarationAvailable=true;unsigned declarationAcquires;
namespace bo1::vulkan::vertex_assets {Decl* Acquire(void*){++declarationAcquires;return declarationAvailable?&vertexDecl:nullptr;}}
struct Arg { unsigned short type{},dest{}; union U { const float* literalConst;unsigned nameHash;struct {unsigned short index;unsigned char firstRow,rowCount;} codeConst;U():nameHash(0){} } u; };
struct Pass { Shader* vertexShader;Shader* pixelShader;void* vertexDecl;unsigned perPrimArgCount,perObjArgCount,stableArgCount,customSamplerFlags;Arg* args; };
struct Tech { unsigned flags,passCount;Pass passArray[2]; };
struct Constant { unsigned nameHash;float literal[4]; };
struct Bits { unsigned loadBits[2]; };
struct TechniqueSet {unsigned worldVertFormat{};};
struct Material {unsigned constantCount,stateBitsCount;Constant* constantTable;Bits* stateBitsTable;unsigned char stateBitsEntry[130];Tech* tech;TechniqueSet* localTechniqueSet{};};
const Tech* Material_GetTechnique(const Material* m,unsigned){return m->tech;}
struct GfxCmdBufInput {float consts[197][4];};
struct {bool supportsIntZ;} dx;
enum {R_RENDERTARGET_SHADOWMAP_SUN=9,CONST_SRC_CODE_SHADOWMAP_POLYGON_OFFSET=20,CONST_SRC_CODE_DESTRUCTIBLE_PARMS=114,CONST_SRC_CODE_COUNT_FLOAT4=197,CONST_SRC_CODE_INVERSE_TRANSPOSE_WORLD_VIEW_PROJECTION_MATRIX=220};
struct Tri {unsigned firstVertex,vertexCount,baseIndex,triCount,vertexLayerData,stream2ByteOffset;};
struct Surface {Tri tris;};
struct Placement {struct {float quat[4],origin[3];} base;float scale;};
struct Constants {float value[7][4];unsigned char constantSource[7],used;};
struct Brush {Surface* surf;Placement* placement;Constants* shaderConstSet;float bmodelBurnAmt,bmodelFadeAmt;};
struct SunShadowMapCallbackUserData {};struct GfxCmdBufState {};
State s_brushCacheState;Mode s_brushCacheMode;unsigned s_brushDiffReports,snapshotBuilds,releases;
std::vector<unsigned char> s_brushEligible;
std::uint64_t s_brushCacheKey;bool s_cacheBrushes,s_brushInvalidated;
unsigned s_brushCount,s_eligibleCount,s_brushSkips,s_farBrushOmitted;bool enabled;
bo1::vulkan::sun_shadow_quality::Policy s_shadowQuality;
namespace bo1::vulkan::pass_recording {
 bool SunBModelCacheEnabled(){return enabled;}
 bool FarBrushShadowsEnabled(){return true;}
 void ReleaseSunShadowCache(){++releases;}
}
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){std::printf("failed line %d\n",__LINE__);return 1;}}while(0)
'''
checks=r'''
int main(){
 // Execute the actual producer's off/transition prefix; snapshot work sentinel
 // is reached only by enabled valid-key calls. No fake backend depth operations.
 for(unsigned i=0;i<100;++i){R_PrepareSunBrushCache(nullptr,7,nullptr);CHECK(s_brushCacheKey==7);CHECK(s_brushCacheMode.Key(7,999)==7);}
 CHECK(snapshotBuilds==0&&releases==0);
 enabled=true;R_PrepareSunBrushCache(nullptr,7,nullptr);CHECK(snapshotBuilds==1&&releases==1);
 R_PrepareSunBrushCache(nullptr,7,nullptr);CHECK(snapshotBuilds==2&&releases==1);
 s_cacheBrushes=true;s_brushEligible={1,1};
 enabled=false;R_PrepareSunBrushCache(nullptr,7,nullptr);CHECK(releases==2&&snapshotBuilds==2);
 CHECK(!s_cacheBrushes&&s_brushEligible.empty()&&s_brushCacheKey==7);
 for(unsigned i=0;i<50;++i)R_PrepareSunBrushCache(nullptr,9,nullptr);
 CHECK(releases==2&&snapshotBuilds==2&&s_brushCacheKey==9);
 enabled=true;R_PrepareSunBrushCache(nullptr,0,nullptr);CHECK(releases==3&&snapshotBuilds==2&&s_brushCacheKey==0);
 R_PrepareSunBrushCache(nullptr,11,nullptr);CHECK(releases==3&&snapshotBuilds==3);
 enabled=false;R_PrepareSunBrushCache(nullptr,11,nullptr);

 Shader vs{1},ps{2};Arg arg{};arg.type=3;arg.u.codeConst={217,0,4};
 Tech tech{0,1,{{&vs,&ps,nullptr,1,0,0,0,&arg}}};
 Constant constant{42,{1,2,3,4}};Bits bits{{1,13}};
 Material material{1,1,&constant,&bits,{},&tech};GfxCmdBufInput input{};
 Values mat;CHECK(R_BrushMaterialSnapshot(&material,5,input,mat));
 Values framed;CHECK(R_BrushMaterialSnapshot(&material,5,input,framed,nullptr,true));
 bits.loadBits[1]|=0x40;CHECK(!R_BrushMaterialSnapshot(&material,5,input,framed,nullptr,true));CHECK(R_BrushMaterialSnapshot(&material,5,input,framed));bits.loadBits[1]&=~0x40u;
 bits.loadBits[1]|=0x80;CHECK(!R_BrushMaterialSnapshot(&material,5,input,framed,nullptr,true));bits.loadBits[1]&=~0x80u;
 auto originalFrequency=mat;tech.passArray[0].perPrimArgCount=0;tech.passArray[0].perObjArgCount=1;
 framed.clear();CHECK(R_BrushMaterialSnapshot(&material,5,input,framed)&&framed!=originalFrequency);tech.passArray[0].perPrimArgCount=1;tech.passArray[0].perObjArgCount=0;
 Surface surf{{3,40,9,10,0,0}};Placement place{{{0,0,0,1},{10,20,30}},1};Constants constants{};
 Brush brush{&surf,&place,nullptr,0,1};
 auto snapshot=[&](){Values m;R_BrushMaterialSnapshot(&material,5,input,m);return BrushValues(m,0x12345678ull,brush);};
 auto original=snapshot();
 Placement copied=place;brush.placement=&copied;CHECK(snapshot()==original);brush.placement=&place;
 CHECK(BrushValues(mat,0x1234abcdull,brush)==original); // frame-local object ID ignored
 State state;Values base{1,2,3};auto first=state.Update(base,{original},true);CHECK(!first.includeBrushes);
 auto stable=state.Update(base,{original},true);CHECK(stable.includeBrushes&&stable.key!=first.key);
 CHECK(state.Update(base,{original},true).key==stable.key);
 auto changed=[&](const Values& value){auto d=state.Update(base,{value},true);return !d.includeBrushes&&d.invalidated&&d.key!=stable.key;};
 place.base.origin[0]+=1;CHECK(changed(snapshot()));
 auto movingKey=state.Update(base,{snapshot()},false).key;
 place.base.origin[0]+=1;auto moving=state.Update(base,{snapshot()},true);CHECK(!moving.includeBrushes&&moving.key==movingKey);
 place.base.origin[0]-=2;state.Update(base,{original},true);stable=state.Update(base,{original},true);CHECK(stable.includeBrushes);
 // The real world-format selector is independently part of the exact signature.
 TechniqueSet selector{0};material.localTechniqueSet=&selector;auto selectorBefore=snapshot();selector.worldVertFormat=4;CHECK(snapshot()!=selectorBefore);
 material.localTechniqueSet=nullptr;tech.flags|=8;Values noSelector;CHECK(!R_BrushMaterialSnapshot(&material,5,input,noSelector));tech.flags&=~8u;
 // Actual backend declaration routing participates, not just the owner address.
 vertexDecl.variants[0].elements[0].offset=4;CHECK(snapshot()!=original);vertexDecl.variants[0].elements[0].offset=0;
 vertexDecl.variants[17].available=false;CHECK(snapshot()!=original);vertexDecl.variants[17].available=true;
 vertexDecl.variants[2].elements.push_back({});CHECK(snapshot()!=original);vertexDecl.variants[2].elements.pop_back();
 declarationAvailable=false;Values absentDecl;CHECK(!R_BrushMaterialSnapshot(&material,5,input,absentDecl));declarationAvailable=true;
 // Every relevant semantic field independently changes the production fingerprint.
 place.base.quat[0]=0.1f;CHECK(snapshot()!=original);place.base.quat[0]=0;
 place.scale=2;CHECK(snapshot()!=original);place.scale=1;
 surf.tris.triCount++;CHECK(snapshot()!=original);surf.tris.triCount--;
 brush.bmodelBurnAmt=0.5f;CHECK(snapshot()!=original);brush.bmodelBurnAmt=0;
 brush.bmodelFadeAmt=0.5f;CHECK(snapshot()!=original);brush.bmodelFadeAmt=1;
 brush.shaderConstSet=&constants;auto custom=snapshot();CHECK(custom!=original);
 constants.constantSource[4]=114;constants.value[4][0]=std::numeric_limits<float>::quiet_NaN();CHECK(snapshot()==custom);
 constants.used=1;CHECK(snapshot()!=custom);custom=snapshot();constants.value[0][0]=7;CHECK(snapshot()!=custom);
 custom=snapshot();constants.constantSource[0]=114;CHECK(snapshot()!=custom);
 constants.value[0][0]=std::numeric_limits<float>::quiet_NaN();CHECK(snapshot().empty());constants.value[0][0]=7;
 State inactive;auto normalized=snapshot();inactive.Update(base,{normalized},true);auto cached=inactive.Update(base,{normalized},true);CHECK(cached.includeBrushes);
 constants.value[6][2]=19;constants.constantSource[6]=32;CHECK(inactive.Update(base,{snapshot()},true).key==cached.key);
 constants.value[0][0]=8;auto activeChange=inactive.Update(base,{snapshot()},true);CHECK(activeChange.invalidated&&!activeChange.includeBrushes);
 brush.shaderConstSet=nullptr;
 // Actual production technique scan determines whether burn/fade are inputs.
 bool consumes=true;Values dependency;CHECK(R_BrushMaterialSnapshot(&material,5,input,dependency,&consumes)&&!consumes);
 auto withoutBurn=BrushValues(dependency,0x12345678ull,brush,consumes);
 brush.bmodelBurnAmt=std::numeric_limits<float>::quiet_NaN();brush.bmodelFadeAmt=0.25f;
 CHECK(BrushValues(dependency,0x12345678ull,brush,consumes)==withoutBurn);
 arg.u.codeConst={114,0,1};dependency.clear();CHECK(R_BrushMaterialSnapshot(&material,5,input,dependency,&consumes)&&consumes);
 CHECK(BrushValues(dependency,0x12345678ull,brush,consumes).empty());brush.bmodelBurnAmt=0;
 auto withBurn=BrushValues(dependency,0x12345678ull,brush,consumes);brush.bmodelBurnAmt=0.5f;CHECK(BrushValues(dependency,0x12345678ull,brush,consumes)!=withBurn);
 brush.bmodelBurnAmt=0;brush.bmodelFadeAmt=1;arg.u.codeConst={217,0,4};
 Arg secondArg{};secondArg.type=7;secondArg.u.codeConst={114,0,1};tech.passArray[1]=tech.passArray[0];tech.passArray[1].args=&secondArg;tech.passCount=2;
 dependency.clear();CHECK(R_BrushMaterialSnapshot(&material,5,input,dependency,&consumes)&&consumes);
 secondArg.u.codeConst={114,1,1};dependency.clear();CHECK(!R_BrushMaterialSnapshot(&material,5,input,dependency,&consumes));
 secondArg.u.codeConst={32,0,1};dependency.clear();CHECK(!R_BrushMaterialSnapshot(&material,5,input,dependency,&consumes));tech.passCount=1;

 constant.literal[0]+=1;CHECK(snapshot()!=original);constant.literal[0]-=1;
 vs.serial++;CHECK(snapshot()!=original);vs.serial--;
 ps.serial++;CHECK(snapshot()!=original);ps.serial--;
 bits.loadBits[0]++;CHECK(snapshot()!=original);bits.loadBits[0]--;
 material.stateBitsEntry[5]++;CHECK(snapshot()!=original);material.stateBitsEntry[5]--;
 arg.type=2;Values rejected;CHECK(!R_BrushMaterialSnapshot(&material,5,input,rejected));
 arg.type=3;arg.u.codeConst={32,0,1};CHECK(!R_BrushMaterialSnapshot(&material,5,input,rejected));
 arg.u.codeConst={20,0,1};CHECK(R_BrushMaterialSnapshot(&material,5,input,rejected));
 arg.u.codeConst={114,0,1};CHECK(R_BrushMaterialSnapshot(&material,5,input,rejected));
 arg.u.codeConst={217,2,4};CHECK(!R_BrushMaterialSnapshot(&material,5,input,rejected));
 arg.u.codeConst={221,0,4};CHECK(!R_BrushMaterialSnapshot(&material,5,input,rejected));
 arg.u.codeConst={20,1,1};CHECK(!R_BrushMaterialSnapshot(&material,5,input,rejected));
 place.scale=std::numeric_limits<float>::quiet_NaN();CHECK(snapshot().empty());place.scale=1;
 arg.u.codeConst={217,0,4};
 // Optional declaration dedup uses one per-build exact dictionary, never cross-frame.
 using DeclDictionary=bo1::vulkan::near_shadow_cache::Declarations;
 DeclDictionary inlineDecls(false),dedupDecls(true);Values inlineA,inlineB,dedupA,dedupB;Material sharedMaterial=material;
 declarationAcquires=0;
 CHECK(R_BrushMaterialSnapshot(&material,5,input,inlineA,nullptr,true,&inlineDecls));
 CHECK(R_BrushMaterialSnapshot(&sharedMaterial,5,input,inlineB,nullptr,true,&inlineDecls));CHECK(declarationAcquires==2&&inlineDecls.UniqueCount()==1);
 declarationAcquires=0;
 CHECK(R_BrushMaterialSnapshot(&material,5,input,dedupA,nullptr,true,&dedupDecls));
 CHECK(R_BrushMaterialSnapshot(&sharedMaterial,5,input,dedupB,nullptr,true,&dedupDecls));CHECK(declarationAcquires==1&&dedupDecls.UniqueCount()==1);
 Values inlineBase,dedupBase;const auto inlineBytes=inlineDecls.Finish(inlineBase);const auto dedupBytes=dedupDecls.Finish(dedupBase);
 CHECK(inlineBase.empty()&&!dedupBase.empty()&&dedupBytes<inlineBytes);CHECK(dedupA.size()<inlineA.size());
 DeclDictionary reverseDecls(true);Values reverseA,reverseB;
 CHECK(R_BrushMaterialSnapshot(&sharedMaterial,5,input,reverseB,nullptr,true,&reverseDecls));
 CHECK(R_BrushMaterialSnapshot(&material,5,input,reverseA,nullptr,true,&reverseDecls));Values reverseDeclBase;reverseDecls.Finish(reverseDeclBase);
 CHECK(reverseDeclBase==dedupBase&&reverseA==dedupA&&reverseB==dedupB);
 bo1::vulkan::near_shadow_cache::State declState;CHECK(!declState.Update(dedupBase,{dedupA,dedupB}).ready);CHECK(declState.Update(dedupBase,{dedupA,dedupB}).ready);
 // Reacquire and copy next preparation; same-owner mutation changes dictionary, not references.
 vertexDecl.variants[17].elements[0].usageIndex=3;DeclDictionary changedDecls(true);Values changedA,changedB;
 CHECK(R_BrushMaterialSnapshot(&material,5,input,changedA,nullptr,true,&changedDecls));CHECK(R_BrushMaterialSnapshot(&sharedMaterial,5,input,changedB,nullptr,true,&changedDecls));
 Values changedDeclBase;changedDecls.Finish(changedDeclBase);CHECK(changedDeclBase!=dedupBase&&changedA==dedupA&&changedB==dedupB);
 CHECK(!declState.Update(changedDeclBase,{changedA,changedB}).ready);vertexDecl.variants[17].elements[0].usageIndex=0;
 int ownerA{},ownerB{};auto savedDeclOwner=tech.passArray[0].vertexDecl;DeclDictionary twoOwners(true),twoOwnersReverse(true);Values ownerValuesA,ownerValuesB,ownerReverseA,ownerReverseB;
 tech.passArray[0].vertexDecl=&ownerA;CHECK(R_BrushMaterialSnapshot(&material,5,input,ownerValuesA,nullptr,true,&twoOwners));
 tech.passArray[0].vertexDecl=&ownerB;CHECK(R_BrushMaterialSnapshot(&material,5,input,ownerValuesB,nullptr,true,&twoOwners));CHECK(twoOwners.UniqueCount()==2&&ownerValuesA!=ownerValuesB);
 CHECK(R_BrushMaterialSnapshot(&material,5,input,ownerReverseB,nullptr,true,&twoOwnersReverse));tech.passArray[0].vertexDecl=&ownerA;
 CHECK(R_BrushMaterialSnapshot(&material,5,input,ownerReverseA,nullptr,true,&twoOwnersReverse));Values twoBase,twoBaseReverse;twoOwners.Finish(twoBase);twoOwnersReverse.Finish(twoBaseReverse);
 CHECK(twoBase==twoBaseReverse&&ownerValuesA==ownerReverseA&&ownerValuesB==ownerReverseB);tech.passArray[0].vertexDecl=savedDeclOwner;
 // Missing declarations remain ineligible; repeated dictionary access does not retry.
 declarationAvailable=false;declarationAcquires=0;DeclDictionary failedDecls(true);Values failedA,failedB;
 CHECK(!R_BrushMaterialSnapshot(&material,5,input,failedA,nullptr,true,&failedDecls));CHECK(!R_BrushMaterialSnapshot(&sharedMaterial,5,input,failedB,nullptr,true,&failedDecls));
 CHECK(declarationAcquires==1&&failedDecls.UniqueCount()==0);declarationAvailable=true;
 // Production dedup keeps definitions exact, deterministic and distinct from brush identity.
 std::unordered_map<const Material*,MaterialSnapshot> definitions;
 Values definition;bool usedDestructible{};CHECK(R_BrushMaterialSnapshot(&material,5,input,definition,&usedDestructible));
 unsigned builds=0;shaderAcquires=0;
 auto buildDefinition=[&](MaterialSnapshot& d){++builds;return R_BrushMaterialSnapshot(&material,5,input,d.values,&d.usesDestructible);};
 for(unsigned i=0;i<759;++i)FindOrBuildMaterialSnapshot(definitions,&material,buildDefinition);
 CHECK(builds==1&&shaderAcquires==2&&definitions.size()==1);
 std::unordered_map<const Material*,MaterialSnapshot> failures;arg.type=2;builds=0;shaderAcquires=0;
 for(unsigned i=0;i<759;++i)CHECK(FindOrBuildMaterialSnapshot(failures,&material,buildDefinition).values.empty());
 CHECK(builds==1&&shaderAcquires==2&&failures.size()==1);arg.type=3;
 definitions.emplace(&material,MaterialSnapshot{definition,usedDestructible});
 Material otherMaterial=material;Values otherDefinition;CHECK(R_BrushMaterialSnapshot(&otherMaterial,5,input,otherDefinition));
 definitions.emplace(&otherMaterial,MaterialSnapshot{otherDefinition,false});
 Values definitionBase{1,2,3};AppendMaterialDefinitions(definitionBase,definitions);
 CHECK(definitionBase[3]==2);
 std::unordered_map<const Material*,MaterialSnapshot> reverseDefinitions;
 reverseDefinitions.emplace(&otherMaterial,MaterialSnapshot{otherDefinition,false});reverseDefinitions.emplace(&material,MaterialSnapshot{definition,usedDestructible});
 Values reverseBase{1,2,3};AppendMaterialDefinitions(reverseBase,reverseDefinitions);CHECK(reverseBase==definitionBase);
 Material rejectedMaterial=material;reverseDefinitions.emplace(&rejectedMaterial,MaterialSnapshot{});
 reverseBase={1,2,3};AppendMaterialDefinitions(reverseBase,reverseDefinitions);CHECK(reverseBase==definitionBase);
 auto compact=BrushValues(Values{reinterpret_cast<std::uintptr_t>(&material)},0x12345678ull,brush,usedDestructible);
 auto otherCompact=BrushValues(Values{reinterpret_cast<std::uintptr_t>(&otherMaterial)},0x12345678ull,brush,usedDestructible);CHECK(compact!=otherCompact);
 CHECK(compact.size()<original.size());
 CHECK(definitions.size()==2);
 State dedup;dedup.Update(definitionBase,{compact,compact,otherCompact},true);
 auto dedupReady=dedup.Update(definitionBase,{otherCompact,compact,compact},true);CHECK(dedupReady.includeBrushes);
 CHECK(dedup.Update(reverseBase,{compact,otherCompact,compact},true).key==dedupReady.key);
 constant.literal[0]+=1;Values updated;CHECK(R_BrushMaterialSnapshot(&material,5,input,updated));constant.literal[0]-=1;
 reverseDefinitions[&material].values=updated;reverseBase={1,2,3};AppendMaterialDefinitions(reverseBase,reverseDefinitions);
 auto materialChange=dedup.Update(reverseBase,{compact,compact,otherCompact},true);CHECK(materialChange.invalidated&&!materialChange.includeBrushes&&materialChange.baseDifference.changed);
 CHECK(dedup.Update(reverseBase,{compact,compact,otherCompact},true).includeBrushes);
 CHECK(!materialChange.brushDifference.changed); // material contents changed, brush values did not
 // Different definition lengths and set identities cannot alias via concatenation.
 reverseDefinitions[&material].values=definition;reverseDefinitions[&material].values.push_back(99);
 reverseBase={1,2,3};AppendMaterialDefinitions(reverseBase,reverseDefinitions);CHECK(reverseBase!=definitionBase);
 reverseDefinitions.erase(&material);reverseDefinitions[&rejectedMaterial].values=definition;
 reverseBase={1,2,3};AppendMaterialDefinitions(reverseBase,reverseDefinitions);CHECK(reverseBase!=definitionBase);
 definitions[&material].values={1};definitions[&otherMaterial].values={2,3};Values framedA;AppendMaterialDefinitions(framedA,definitions);
 definitions[&material].values={1,2};definitions[&otherMaterial].values={3};Values framedB;AppendMaterialDefinitions(framedB,definitions);CHECK(framedA!=framedB);
 definitions.erase(&otherMaterial);Values fewer;AppendMaterialDefinitions(fewer,definitions);CHECK(fewer[0]==1&&fewer!=framedB);
 // Reorder is harmless; multiplicity/addition/removal remain significant.
 State set;auto second=original;second[0]++;
 set.Update(base,{original,second},true);auto both=set.Update(base,{second,original},true);CHECK(both.includeBrushes);
 CHECK(set.Update(base,{original,second},true).key==both.key);
 auto removed=set.Update(base,{original},true);CHECK(!removed.includeBrushes&&removed.key!=both.key);
 CHECK(set.Update(base,{original},true).includeBrushes);
 CHECK(!set.Update(base,{},true).includeBrushes);
 CHECK(!set.Update(base,{original,original},true).includeBrushes);
 CHECK(set.Update(base,{original,original},true).includeBrushes);
 auto oldKey=set.Update(base,{original,original},true).key;base[1]++; // world buffer/device generation
 auto reload=set.Update(base,{original,original},true);CHECK(!reload.includeBrushes&&reload.key!=oldKey);CHECK(reload.baseDifference.changed&&reload.baseDifference.index==1&&reload.baseDifference.before==2&&reload.baseDifference.after==3);
 CHECK(set.Update(base,{original,original},true).includeBrushes);
 CHECK(!set.Update(base,{original,original},false).includeBrushes);
 CHECK(!set.Update(base,{original,original},true).includeBrushes);
 CHECK(set.Update(base,{original,original},true).includeBrushes);
 std::printf("PASS %u production material/fingerprint/cache-state checks\n",checks);return 0;
}
'''
fixture=out/"fixture.cpp";fixture.write_text(prefix+material+off_path+checks,encoding="ascii")
exe=out/"fixture.exe"
subprocess.run([r"C:\Program Files\LLVM\bin\clang++.exe","-std=c++17","-O2","-I",str(root/"src"),str(fixture),"-o",str(exe)],check=True)
subprocess.run([str(exe)],check=True)
