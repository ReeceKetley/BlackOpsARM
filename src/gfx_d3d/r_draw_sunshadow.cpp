#include "r_draw_sunshadow.h"
#include "r_state_utils.h"
#include "r_wind.h"
#include "r_state.h"
#include "r_foliage.h"
#include "r_dvars.h"
#include "rb_draw3d.h"
#include "rb_spotshadow.h"
#if defined(__ANDROID__)
#include "gfx_vulkan/material_dispatch.h"
#include <android/log.h>
#endif
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
#include "r_sunshadow.h"
#include "gfx_vulkan/pass_recording.h"
#include "gfx_vulkan/shader_assets.h"
#include "gfx_vulkan/vertex_assets.h"
#include "gfx_vulkan/buffer_assets.h"
#include "gfx_vulkan/sun_bmodel_cache.h"
#include "gfx_vulkan/sun_shadow_quality.h"
#include "gfx_vulkan/near_shadow_cache.h"
#include "r_staticmodelcache.h"
#include "r_vertexstream2.h"
#include <universal/narrow32.h>
#include "r_shader_constant_set.h"
#include "r_scene.h"
#include <unordered_map>
#include <chrono>
#include <string>
#include <vector>
namespace {
// Render thread: whether this frame's far partition received the cached static
// casters (restored before the near partition's clear).
bool s_sunShadowFarRestored;
VkRect2D R_SunShadowFarRect(const GfxBackEndData *data)
{
    const GfxViewport &v = data->sunShadow.partition[1].viewport;
    return VkRect2D{ { v.x, v.y + (int)dx.sunShadowmapSize }, { (unsigned)v.width, (unsigned)v.height } };
}
using namespace bo1::vulkan::sun_bmodel_cache;
State s_brushCacheState;
Mode s_brushCacheMode;
bo1::vulkan::sun_shadow_quality::Policy s_shadowQuality;
unsigned s_farBrushOmitted;
unsigned s_brushDiffReports;
std::uint64_t s_brushCacheKey;
bool s_cacheBrushes;
std::vector<unsigned char> s_brushEligible;
unsigned s_brushCount, s_eligibleCount, s_brushSkips;
bool s_brushInvalidated;
// All references are consumed on the render thread from the handed-off frame.
// Only copied scalar values survive; objectId/placement pointers are frame-local.
bool R_BrushMaterialSnapshot(const Material* material,unsigned techType,const GfxCmdBufInput& input,Values& out,bool* usesDestructible=nullptr,bool depthOnly=false,bo1::vulkan::near_shadow_cache::Declarations* declarations=nullptr)
{
    if(usesDestructible)*usesDestructible=false;
    // Match the canonical float-Z/int-Z shadow caster technique selection.
    if(dx.supportsIntZ && R_RENDERTARGET_SHADOWMAP_SUN!=22){if(techType==5)techType=7;else if(techType==6)techType=8;}
    const auto* tech=Material_GetTechnique(material,techType);
    if(!tech || (tech->flags&0x20) || !tech->passCount)return false;
    out={reinterpret_cast<std::uintptr_t>(material),reinterpret_cast<std::uintptr_t>(tech),techType,tech->flags,tech->passCount,material->constantCount,material->stateBitsCount};
    // R_SetVertexDeclTypeWorld selects worldVertFormat+2 for technique flag8.
    // All variant contents alone do not capture a changed selector.
    if((tech->flags&8)&&!material->localTechniqueSet)return false;
    out.push_back(material->localTechniqueSet?std::uint64_t(material->localTechniqueSet->worldVertFormat):~std::uint64_t(0));
    for(unsigned i=0;i<material->constantCount;++i){if(!Finite(material->constantTable[i].literal,4))return false;out.push_back(material->constantTable[i].nameHash);Floats(out,material->constantTable[i].literal,4);}
    for(unsigned i=0;i<material->stateBitsCount;++i){out.push_back(material->stateBitsTable[i].loadBits[0]);out.push_back(material->stateBitsTable[i].loadBits[1]);}
    for(unsigned i=0;i<130;++i)out.push_back(material->stateBitsEntry[i]);
    if(depthOnly){
        const unsigned first=material->stateBitsEntry[techType];
        if(first>material->stateBitsCount||tech->passCount>unsigned(material->stateBitsCount)-first)return false;
        for(unsigned p=0;p<tech->passCount;++p)if(!bo1::vulkan::near_shadow_cache::DepthPassSafe(material->stateBitsTable[first+p].loadBits[1]))return false;
    }
    for(unsigned p=0;p<tech->passCount;++p){
        const auto& pass=tech->passArray[p];
        out.push_back(pass.perPrimArgCount);out.push_back(pass.perObjArgCount);out.push_back(pass.stableArgCount);out.push_back(pass.customSamplerFlags);
        const auto vs=bo1::vulkan::shader_assets::Acquire(pass.vertexShader);
        const auto ps=bo1::vulkan::shader_assets::Acquire(pass.pixelShader);
        if(!vs || !ps || pass.customSamplerFlags)return false;
        out.push_back(vs->serial);out.push_back(ps->serial);
        out.push_back(reinterpret_cast<std::uintptr_t>(pass.vertexDecl));
        auto appendDeclaration=[&](Values& values){
            const auto declaration=bo1::vulkan::vertex_assets::Acquire(pass.vertexDecl);
            if(!declaration)return false;
            // Copy all actual backend variants, never a transient record pointer.
            values.push_back(declaration->variants.size());
            for(unsigned v=0;v<declaration->variants.size();++v){const auto& variant=declaration->variants[v];
                values.push_back(v);values.push_back(variant.available);values.push_back(variant.elements.size());
                for(const auto& element:variant.elements){values.push_back(element.stream);values.push_back(element.offset);
                    values.push_back(element.type);values.push_back(element.usage);values.push_back(element.usageIndex);}}
            return true;
        };
        out.push_back(declarations&&declarations->enabled); // frame inline vs dictionary representation
        if(declarations){if(!declarations->Append(out,pass.vertexDecl,appendDeclaration))return false;}
        else if(!appendDeclaration(out))return false;
        const unsigned count=pass.perPrimArgCount+pass.perObjArgCount+pass.stableArgCount;
        for(unsigned i=0;i<count;++i){const auto& arg=pass.args[i];out.push_back(arg.type);out.push_back(arg.dest);
            switch(arg.type){
            case 0:case 6:out.push_back(arg.u.nameHash);break; // values copied above
            case 1:case 5:if(!arg.u.literalConst||!Finite(arg.u.literalConst,4))return false;Floats(out,arg.u.literalConst,4);break;
            case 3:case 7:{const auto route=arg.u.codeConst;
                out.push_back(route.index);out.push_back(route.firstRow);out.push_back(route.rowCount);
                if(route.index<CONST_SRC_CODE_COUNT_FLOAT4){
                    // These are reconstructed per object/pass, not copied from
                    // viewInfo.input. Their exact producers are fingerprinted
                    // below (burn/fade/custom sets) and in the base (bias/scale).
                    if(route.firstRow || route.rowCount!=1 || (route.index!=CONST_SRC_CODE_SHADOWMAP_POLYGON_OFFSET
                        &&route.index!=CONST_SRC_CODE_DESTRUCTIBLE_PARMS))return false;
                    if(route.index==CONST_SRC_CODE_DESTRUCTIBLE_PARMS&&usesDestructible)*usesDestructible=true;
                }else if(route.index>CONST_SRC_CODE_INVERSE_TRANSPOSE_WORLD_VIEW_PROJECTION_MATRIX
                    ||!route.rowCount||route.firstRow+route.rowCount>4)return false;
                break;}
            default:return false; // animated/feedback textures stay dynamic
            }
        }
    }
    return true;
}
const char* R_BrushBaseField(std::size_t i)
{
    if(i<4){static const char* names[]={"frontend_key","release_generation","world_pointer","world_vb_serial"};return names[i];}
    if(i<36)return ((i-4)%8)<4?"view_projection":"projection";
    static const char* names[]={"viewport_x","viewport_y","viewport_w","viewport_h","burn_color","sun_bias","sun_scale","polygon_bias","polygon_scale","depth_type","depth_near","depth_far"};
    return i<48?names[i-36]:"material_definitions";
}
const char* R_BrushValueField(std::size_t i,std::size_t start)
{
    if(i<start)return "material_identity";
    static const char* names[]={"draw_key","surface","first_vertex","vertex_count","base_index","tri_count","vertex_layer","stream_offset",
        "quat_x","quat_y","quat_z","quat_w","origin_x","origin_y","origin_z","scale","burn","fade","has_constants"};
    return i-start<19?names[i-start]:"custom_constants_or_size";
}
void R_LogSunBrushDifference(const Decision& d)
{
    if(!(d.baseDifference.changed||d.brushDifference.changed)||s_brushDiffReports>=20)return;
    ++s_brushDiffReports;
    const auto& b=d.baseDifference;const auto& v=d.brushDifference;
    float before{},after{};const unsigned beforeBits=(unsigned)b.before,afterBits=(unsigned)b.after;
    std::memcpy(&before,&beforeBits,4);std::memcpy(&after,&afterBits,4);
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "sun_bmodel_diff sample=%u base_changed=%u base_index=%u base_field=%s base_before=%llx base_after=%llx base_float=%.9g,%.9g brush_changed=%u old_count=%u new_count=%u brush_index=%u value_index=%u value_field=%s value_before=%llx value_after=%llx old_surface=%llx new_surface=%llx",
        s_brushDiffReports,unsigned(b.changed),unsigned(b.index),R_BrushBaseField(b.index),(unsigned long long)b.before,(unsigned long long)b.after,before,after,
        unsigned(v.changed),unsigned(d.beforeCount),unsigned(d.afterCount),unsigned(d.brushIndex),unsigned(v.index),R_BrushValueField(v.index,d.semanticStart),
        (unsigned long long)v.before,(unsigned long long)v.after,(unsigned long long)d.beforeSurface,(unsigned long long)d.afterSurface);
}
void R_PrepareSunBrushCache(const SunShadowMapCallbackUserData* data,std::uint64_t baseKey,const GfxCmdBufState* state)
{
    const bool farBrushes=bo1::vulkan::pass_recording::FarBrushShadowsEnabled();
    const bool enabled=farBrushes&&bo1::vulkan::pass_recording::SunBModelCacheEnabled();
    const bool modeChanged=s_brushCacheMode.Change(enabled);
    const bool changed=s_shadowQuality.Update(farBrushes,modeChanged);
    s_farBrushOmitted=0;
    if(changed){
        // A brush-inclusive image must not alias the original frontend key,
        // and original/extended keys both start at small integer serials.
        bo1::vulkan::pass_recording::ReleaseSunShadowCache();
        s_brushCacheState=State{};s_brushEligible.clear();s_brushDiffReports=0;
    }
    if(!enabled||!baseKey){
        // Preserve the original static-far-cache policy exactly while off.
        // No world acquisition, per-brush allocation, material lookup or sort.
        s_brushCacheKey=baseKey;s_cacheBrushes=false;s_brushEligible.clear();
        s_brushCount=s_eligibleCount=s_brushSkips=0;s_brushInvalidated=changed;
        return;
    }
    const auto* frame=data->data;
    const auto& far=frame->sunShadow.partition[1];
    const auto& list=data->drawList->viewInfo->drawList[DRAWLIST_SUN_SHADOW_MAP0+1];
    Values base{baseKey,bo1::vulkan::pass_recording::SunShadowCacheGeneration(),reinterpret_cast<std::uintptr_t>(rgp.world)};
    const auto world=bo1::vulkan::buffer_assets::Acquire(&rgp.world->draw.vd.worldVb);
    base.push_back(world?world->serial:0);
    bool finiteBase=true;
    for(unsigned row=0;row<4;++row){finiteBase=finiteBase&&Finite(far.shadowViewParms.viewProjectionMatrix.m[row],4)&&Finite(far.shadowViewParms.projectionMatrix.m[row],4);Floats(base,far.shadowViewParms.viewProjectionMatrix.m[row],4);Floats(base,far.shadowViewParms.projectionMatrix.m[row],4);}
    base.push_back(far.viewport.x);base.push_back(far.viewport.y);base.push_back(far.viewport.width);base.push_back(far.viewport.height);
    Floats(base,&r_burnedDestructibleColor->current.value,1);
    Floats(base,&sm_polygonOffsetBias->current.value,1);Floats(base,&sm_polygonOffsetScale->current.value,1);
    Floats(base,&r_polygonOffsetBias->current.value,1);Floats(base,&r_polygonOffsetScale->current.value,1);
    finiteBase=finiteBase&&Finite(&r_burnedDestructibleColor->current.value,1)&&Finite(&sm_polygonOffsetBias->current.value,1)&&Finite(&sm_polygonOffsetScale->current.value,1);
    finiteBase=finiteBase&&Finite(&r_polygonOffsetBias->current.value,1)&&Finite(&r_polygonOffsetScale->current.value,1);
    // Both partition callbacks receive freshly initialized local state from
    // R_DrawCall. Include its real viewport depth mapping, not a fixed guess.
    base.push_back(state->depthRangeType);
    Floats(base,&state->depthRangeNear,1);Floats(base,&state->depthRangeFar,1);
    finiteBase=finiteBase&&Finite(&state->depthRangeNear,1)&&Finite(&state->depthRangeFar,1);
    Snapshot snapshot;
    s_brushEligible.assign(list.drawSurfCount,0);s_brushCount=s_eligibleCount=s_brushSkips=0;
    std::unordered_map<const Material*,MaterialSnapshot> materials;
    for(unsigned i=0;i<list.drawSurfCount;++i){const auto draw=list.drawSurfs[i];if(draw.fields.surfType!=SF_BMODEL)continue;
        ++s_brushCount;
        const auto* brush=reinterpret_cast<const BModelSurface*>(reinterpret_cast<const char*>(frame)+4*draw.fields.objectId);
        const auto* material=brush->surf->material;
        const auto& definition=FindOrBuildMaterialSnapshot(materials,material,[&](MaterialSnapshot& snapshot){
            return R_BrushMaterialSnapshot(material,list.baseTechType,list.viewInfo->input,snapshot.values,&snapshot.usesDestructible);
        });
        if(definition.values.empty())continue;
        Values values=BrushValues(Values{reinterpret_cast<std::uintptr_t>(material)},draw.packed,*brush,definition.usesDestructible);
        if(values.empty())continue;
        snapshot.push_back(std::move(values));s_brushEligible[i]=1;++s_eligibleCount;
    }
    AppendMaterialDefinitions(base,materials);
    const auto decision=s_brushCacheState.Update(std::move(base),std::move(snapshot),
        finiteBase&&world&&baseKey&&bo1::vulkan::pass_recording::SunBModelCacheEnabled());
    R_LogSunBrushDifference(decision);
    s_brushCacheKey=decision.key;s_cacheBrushes=decision.includeBrushes;s_brushInvalidated=decision.invalidated;
}
// Near cache uses only the current ordinary-frustum draw stream. No caster
// expansion and no cached actor/brush/skinned/pretess geometry.
bo1::vulkan::near_shadow_cache::State s_nearState;
bo1::vulkan::near_shadow_cache::Decision s_nearDecision;
bool s_nearEnabled,s_nearRestored;
std::vector<unsigned char> s_nearEligible;
unsigned s_nearEntries,s_nearStaticDraws,s_nearDynamicDraws,s_nearRejected,s_nearLastStoredDraws;
double s_nearPrepareMs;
std::size_t s_nearSerializedBytes,s_nearDeclarationBytes,s_nearGeometryBytes;
unsigned s_nearUniqueDeclarations;bool s_nearDeclDedup;
unsigned s_nearMaterialCount,s_nearDefinitionCount;
VkRect2D R_NearRect(const GfxBackEndData* frame){const auto& v=frame->sunShadow.partition[0].viewport;
    return {{v.x,v.y},{unsigned(v.width),unsigned(v.height)}};}
bool R_NearOwner(Values& values,const void* owner){
    const auto record=bo1::vulkan::buffer_assets::Acquire(owner);if(!record)return false;
    values.push_back(reinterpret_cast<std::uintptr_t>(owner));values.push_back(record->serial);return true;
}
bool R_NearGeometry(unsigned type,unsigned handle,unsigned id,Values& values,const GfxDrawSurfListInfo& list){
    const auto* world=rgp.world;
    if(type==0){
        if(id>=world->surfaceCount)return false;
        const auto& tris=world->dpvs.surfaces[id].tris;
        values.push_back(id);values.push_back(tris.firstVertex);values.push_back(tris.vertexCount);
        values.push_back(tris.baseIndex);values.push_back(tris.triCount);values.push_back(tris.vertexLayerData);values.push_back(tris.stream2ByteOffset);
        // World indices are written only at level load (r_bsp_load_obj) and
        // rgp.world is part of the base key: the range itself identifies them.
        return world->draw.indices&&world->draw.indexCount>=0
            &&std::uint64_t(tris.baseIndex)+3ull*tris.triCount<=std::uint64_t(world->draw.indexCount);
    }
    unsigned instance=id;
    if(type==4){
        // 512 trees * 32 leaves, canonical indices are 1-based. No native
        // pointer-size assumptions; SMC_GetLeaf performs the real mapping.
        if(!id||id>512u*32u)return false;
        const auto& leaf=SMC_GetLeaf(id)->cachedSurf;instance=leaf.smodelIndex;
        if(instance>=world->dpvs.smodelCount||leaf.lodIndex>=4)return false;
        if(world->dpvs.smodelDrawInsts[instance].smodelCacheIndex[leaf.lodIndex]!=id)return false;
        values.push_back(id);values.push_back(leaf.baseVertIndex);values.push_back(leaf.lodIndex);
    }
    if(instance>=world->dpvs.smodelCount)return false;
    if(!list.viewInfo->dynSModelState||instance<list.viewInfo->dynSModelState->maxDynSModelIndexInMap)return false;
    const auto& inst=world->dpvs.smodelDrawInsts[instance];const auto* model=inst.model;
    if(!model||!model->surfs||!model->numsurfs)return false;
    const auto address=reinterpret_cast<std::uintptr_t>(BO1_NARROW_DECODE(XSurface,handle));
    const auto first=reinterpret_cast<std::uintptr_t>(model->surfs);
    if(address<first||address-first>=std::size_t(model->numsurfs)*sizeof(XSurface)||(address-first)%sizeof(XSurface))return false;
    const auto* surface=reinterpret_cast<const XSurface*>(address);
    if(!Finite(inst.placement.origin,3)||!Finite(&inst.placement.axis[0][0],9)||!Finite(&inst.placement.scale,1))return false;
    values.push_back(instance);values.push_back(reinterpret_cast<std::uintptr_t>(model));values.push_back(address);
    values.push_back(surface->vertCount);values.push_back(surface->triCount);values.push_back(surface->baseTriIndex);values.push_back(surface->baseVertIndex);values.push_back(surface->flags);
    Floats(values,inst.placement.origin,3);Floats(values,&inst.placement.axis[0][0],9);Floats(values,&inst.placement.scale,1);
    if(type==4){
        const auto& leaf=SMC_GetLeaf(id)->cachedSurf;const auto& lod=model->lodInfo[leaf.lodIndex];
        const auto surfIndex=(address-first)/sizeof(XSurface);
        if(surfIndex<lod.surfIndex||surfIndex>=unsigned(lod.surfIndex)+lod.numsurfs)return false;
        // The cache VB serial changes on upload/eviction replacement. Its
        // exact current leaf mapping also participates above.
        return R_NearOwner(values,&gfxBuf.smodelCacheVb)&&bo1::vulkan::near_shadow_cache::AppendIndices(values,gfxBuf.smodelCache.indices,
            0x100000u,3ull*surface->baseTriIndex+4ull*leaf.baseVertIndex,3ull*surface->triCount);
    }
    return R_NearOwner(values,&surface->vb0)&&R_NearOwner(values,&surface->indexBuffer);
}
void R_PrepareNearCache(const SunShadowMapCallbackUserData* data,const GfxCmdBufState* state){
    const bool enabled=bo1::vulkan::pass_recording::NearShadowCacheEnabled()&&bo1::vulkan::shader_assets::Active();
    if(enabled!=s_nearEnabled){bo1::vulkan::pass_recording::ReleaseNearShadowCache();s_nearState.Reset();s_nearLastStoredDraws=0;}
    s_nearEnabled=enabled;s_nearRestored=false;s_nearDecision={};s_nearEligible.clear();
    s_nearEntries=s_nearStaticDraws=s_nearDynamicDraws=s_nearRejected=0;
    if(!enabled)return;
    // Two clock reads for the whole enabled preparation, including early
    // rejection and local snapshot destruction. No per-entry timing or off work.
    struct PreparationTimer {
        std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
        ~PreparationTimer(){s_nearPrepareMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();}
    } preparationTimer;
    s_nearSerializedBytes=s_nearDeclarationBytes=s_nearGeometryBytes=0;s_nearUniqueDeclarations=0;
    s_nearMaterialCount=s_nearDefinitionCount=0;
    s_nearDeclDedup=bo1::vulkan::pass_recording::NearShadowDeclDedupEnabled();
    bo1::vulkan::near_shadow_cache::Declarations declarations(s_nearDeclDedup);
    const auto* frame=data->data;const auto& partition=frame->sunShadow.partition[0];const auto& list=*data->drawList;
    const auto wordCount=unsigned(frame->primDrawSurfPos);
    if(!rgp.world||g_worldDraw!=&rgp.world->draw||wordCount>sizeof(frame->primDrawSurfsBuf)/sizeof(frame->primDrawSurfsBuf[0])){s_nearState.Reset();return;}
    Values base{bo1::vulkan::pass_recording::SunShadowCacheGeneration(),reinterpret_cast<std::uintptr_t>(rgp.world),unsigned(dx.sunShadowmapSize),unsigned(list.baseTechType)};
    if(!R_NearOwner(base,&rgp.world->draw.vd.worldVb)){s_nearState.Reset();return;}
    // World declarations may route layer and auxiliary streams. Include their
    // actual owner generations, even when a particular shadow pass omits them.
    base.push_back(rgp.world->draw.vertexLayerDataSize);base.push_back(rgp.world->draw.vertexStream2DataSize);
    if((rgp.world->draw.vertexLayerDataSize&&!R_NearOwner(base,&rgp.world->draw.vld.layerVb))
        ||(rgp.world->draw.vertexStream2DataSize&&!R_NearOwner(base,R_VertexStream2_GetVB()))){s_nearState.Reset();return;}
    const auto& view=partition.shadowViewParms;
    for(unsigned row=0;row<4;++row){
        if(!Finite(view.viewMatrix.m[row],4)||!Finite(view.projectionMatrix.m[row],4)||!Finite(view.viewProjectionMatrix.m[row],4)){s_nearState.Reset();return;}
        Floats(base,view.viewMatrix.m[row],4);Floats(base,view.projectionMatrix.m[row],4);Floats(base,view.viewProjectionMatrix.m[row],4);
    }
    const auto& v=partition.viewport;base.push_back(v.x);base.push_back(v.y);base.push_back(v.width);base.push_back(v.height);
    const float constants[]={sm_polygonOffsetBias->current.value,sm_polygonOffsetScale->current.value,r_polygonOffsetBias->current.value,r_polygonOffsetScale->current.value,state->depthRangeNear,state->depthRangeFar};
    if(!Finite(constants,6)){s_nearState.Reset();return;}Floats(base,constants,6);base.push_back(state->depthRangeType);
    Snapshot records;std::unordered_map<const Material*,MaterialSnapshot> materials;s_nearEligible.assign(list.drawSurfCount,0);
    for(unsigned i=0;i<list.drawSurfCount;++i){const auto draw=list.drawSurfs[i];const unsigned type=draw.fields.surfType;
        // Every admitted static caster is cached, wherever it sorts: the
        // admitted materials are depth-only with monotonic depth writes, so
        // the shadow map does not depend on draw order. Actors, brushes and
        // unsupported entries keep the canonical path in their original order.
        if(type!=0&&type!=2&&type!=4)continue;
        const auto* material=rgp.sortedMaterials[draw.fields.materialSortedIndex];
        if(!material){++s_nearRejected;continue;}
        const auto& definition=FindOrBuildMaterialSnapshot(materials,material,[&](MaterialSnapshot& snapshot){
            const auto before=declarations.inlineWords;
            const bool admitted=R_BrushMaterialSnapshot(material,list.baseTechType,list.viewInfo->input,snapshot.values,&snapshot.usesDestructible,true,&declarations)&&!snapshot.usesDestructible;
            if(!admitted)declarations.inlineWords=before; // failed material has no serialized inline definition
            return admitted;
        });
        if(definition.values.empty()){++s_nearRejected;continue;}
        Values values{reinterpret_cast<std::uintptr_t>(material),draw.packed&~std::uint64_t(0xffff)};
        if(!bo1::vulkan::near_shadow_cache::Decode(frame->primDrawSurfsBuf,wordCount,draw.fields.objectId,type,values,
            [&](unsigned t,unsigned surface,unsigned id,Values& output){return R_NearGeometry(t,surface,id,output,list);})){++s_nearRejected;continue;}
        s_nearEligible[i]=1;++s_nearEntries;records.push_back(std::move(values));
    }
    base.push_back(s_nearDeclDedup);AppendMaterialDefinitions(base,materials);
    s_nearDeclarationBytes=declarations.Finish(base);s_nearUniqueDeclarations=declarations.UniqueCount();
    s_nearMaterialCount=unsigned(materials.size());
    for(const auto& material:materials)if(!material.second.values.empty())++s_nearDefinitionCount;
    // Exact final State::Update layout: base, record count, then framed records.
    std::size_t words=base.size()+1;
    for(const auto& record:records)words+=1+record.size();
    s_nearGeometryBytes=(words-base.size())*sizeof(Values::value_type);
    if(s_nearDeclDedup)base.reserve(words); // final concatenation allocates at most once
    s_nearSerializedBytes=words*sizeof(Values::value_type);
    s_nearDecision=s_nearState.Update(std::move(base),std::move(records));
}
void R_DrawNearKind(GfxCmdBufContext context,const GfxDrawSurfListInfo* list,bool eligible){
    thread_local std::vector<GfxDrawSurf> filtered;filtered.clear();
    for(unsigned i=0;i<list->drawSurfCount;++i)if((i<s_nearEligible.size()&&s_nearEligible[i]!=0)==eligible)filtered.push_back(list->drawSurfs[i]);
    if(filtered.empty())return;
    GfxDrawSurfListInfo selected=*list;selected.drawSurfs=filtered.data();selected.drawSurfCount=unsigned(filtered.size());
    const auto before=bo1::vulkan::material_dispatch::ReadProfile().draws;R_DrawSurfs(context,0,&selected);
    const unsigned draws=unsigned(bo1::vulkan::material_dispatch::ReadProfile().draws-before);
    if(eligible)s_nearStaticDraws+=draws;else s_nearDynamicDraws+=draws;
}

// kind: -1 all casters (uncached fallback), 0 static, 1 dynamic.
void R_DrawSunShadowSurfsOfKind(GfxCmdBufContext context, const GfxDrawSurfListInfo *drawList, int kind)
{
    thread_local std::vector<GfxDrawSurf> surfs;
    surfs.clear();
    for (unsigned i = 0; i < drawList->drawSurfCount; ++i){
        const auto draw=drawList->drawSurfs[i];
        if(!bo1::vulkan::sun_shadow_quality::KeepCaster(1,draw.fields.surfType==SF_BMODEL,s_shadowQuality.farBrushes)) {
            if(kind!=0)++s_farBrushOmitted;
            continue;
        }
        const bool cachedBrush=s_cacheBrushes&&i<s_brushEligible.size()&&s_brushEligible[i];
        if(bo1::vulkan::sun_shadow_quality::SelectCaster(1,draw.fields.surfType,SF_BMODEL,s_shadowQuality.farBrushes,kind,cachedBrush))surfs.push_back(draw);
        else if(kind==1&&cachedBrush&&s_sunShadowFarRestored)++s_brushSkips;
    }
    if (surfs.empty())return;
    GfxDrawSurfListInfo filtered = *drawList;
    filtered.drawSurfs = surfs.data();filtered.drawSurfCount = (unsigned)surfs.size();
    R_DrawSurfs(context, 0, &filtered);
}

}
#endif

void    R_DrawSunShadowMap(
                const GfxViewInfo *viewInfo,
                unsigned int partitionIndex,
                GfxCmdBuf *cmdBuf)
{
    float x; // [esp+20h] [ebp-1ABCh]
    const GfxBackEndData *data; // [esp+2Ch] [ebp-1AB0h]
    GfxCmdBufSourceState state; // [esp+30h] [ebp-1AACh] BYREF

    R_InitCmdBufSourceState(&state, &viewInfo->input, 0);
    R_SetWindShaderConstants(&state);
    R_SetRenderTargetSize(&state, R_RENDERTARGET_SHADOWMAP_SUN);

    data = viewInfo->input.data;
    x = data->sunShadow.partition[partitionIndex].shadowViewParms.projectionMatrix.m[2][2];

    R_UpdateCodeConstant(
        &state,
        CONST_SRC_CODE_SHADOWMAP_POLYGON_OFFSET,
        (sm_polygonOffsetBias->current.value * 0.25) * x,
        sm_polygonOffsetScale->current.value,
        0.0,
        0.0);

    R_SetViewportValues(&state, 0, partitionIndex * dx.sunShadowmapSize, dx.sunShadowmapSize, dx.sunShadowmapSize);

    SunShadowMapCallbackUserData userData;
    userData.partition = &data->sunShadow.partition[partitionIndex];
    userData.drawList = &viewInfo->drawList[DRAWLIST_SUN_SHADOW_MAP0 + partitionIndex];
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    userData.data = data;
#endif
    
    R_DrawCall(
        R_DrawSunShadowMapCallback, 
        &userData,
        &state, 
        viewInfo, 
        (GfxDrawSurfListInfo*)&viewInfo->drawList[DRAWLIST_SUN_SHADOW_MAP0 + partitionIndex], // de-const cast
        &data->sunShadow.partition[partitionIndex].shadowViewParms,
        cmdBuf, 
        0);
}

void __cdecl R_DrawSunShadowMapCallback(const void *userData, GfxCmdBufContext context, GfxCmdBufContext prepassContext)
{
    const GfxDrawSurfListInfo *drawList; // [esp+34h] [ebp-8h]
    const GfxSunShadowPartition *partition; // [esp+38h] [ebp-4h]

    const SunShadowMapCallbackUserData *data = (const SunShadowMapCallbackUserData *)userData;

    partition = data->partition;
    drawList = data->drawList;

    R_SetRenderTarget(context, R_RENDERTARGET_SHADOWMAP_SUN);
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    // Cached far partition: its static casters' depth is copied in, then only
    // the near partition is cleared and dynamic casters are drawn over it.
    const unsigned long long baseCacheKey = bo1::vulkan::shader_assets::Active() ? R_SunShadowCacheKey(data->data) : 0;
    if(!partition->partitionIndex){R_PrepareSunBrushCache(data,baseCacheKey,context.state);R_PrepareNearCache(data,context.state);}
    const unsigned long long cacheKey=baseCacheKey?s_brushCacheMode.Key(baseCacheKey,s_brushCacheKey):0;
    if ( !partition->partitionIndex )
    {
        s_sunShadowFarRestored = false;
        // With independent near/far restores, every clear precedes both copies.
        // Off mode preserves the original far-only clear/restore sequence.
        if(s_nearEnabled)R_ClearScreen(context.state->prim.device,3u,shadowmapClearColor,1.0,0,0);
        if ( cacheKey )
        {
            std::string error;
            auto scope = bo1::vulkan::pass_recording::Current();
            if ( !scope || !scope->RestoreSunShadowCache(context.state, R_SunShadowFarRect(data->data), cacheKey, s_sunShadowFarRestored, error) )
                Com_Error(ERR_FATAL, "Native sun shadow cache restore: %s", error.c_str());
        }
        if ( !s_nearEnabled && s_sunShadowFarRestored )
        {
            const GfxViewport nearRegion{ 0, 0, (int)dx.sunShadowmapSize, (int)dx.sunShadowmapSize };
            R_ClearScreen(context.state->prim.device, 3u, shadowmapClearColor, 1.0, 0, &nearRegion);
        }
        else if(!s_nearEnabled)
            R_ClearScreen(context.state->prim.device, 3u, shadowmapClearColor, 1.0, 0, 0);
        if(s_nearEnabled&&s_nearDecision.ready){
            std::string error;auto scope=bo1::vulkan::pass_recording::Current();
            if(!scope||!scope->RestoreSunShadowCache(context.state,R_NearRect(data->data),s_nearDecision.key,s_nearRestored,error,0))
                Com_Error(ERR_FATAL,"Native near shadow cache restore: %s",error.c_str());
        }
    }
#else
    if ( !partition->partitionIndex )
        R_ClearScreen(context.state->prim.device, 3u, shadowmapClearColor, 1.0, 0, 0);
#endif
    R_HW_EnableScissor(
        context.state->prim.device,
        partition->viewport.x,
        partition->viewport.y + partition->partitionIndex * dx.sunShadowmapSize,
        partition->viewport.width,
        partition->viewport.height);
#if defined(__ANDROID__)
    // Diagnostic: per-partition caster composition and the draws it issues.
    static unsigned diagnosticFrame;
    const bool report = partition->partitionIndex < 2 && (diagnosticFrame++ % 200) < 2;
    const unsigned drawsBefore = bo1::vulkan::material_dispatch::ReadProfile().draws;
#endif
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    RB_ParallelDrawScope parallelScope; // r_nativeParallelRecord: caster lists of this partition
    if(!partition->partitionIndex&&s_nearEnabled){
        if(!s_nearRestored){
            R_DrawNearKind(context,drawList,true);
            if(s_nearDecision.ready){
                std::string error;auto scope=bo1::vulkan::pass_recording::Current();
                if(!scope||!scope->StoreSunShadowCache(context.state,R_NearRect(data->data),s_nearDecision.key,error,0))
                    Com_Error(ERR_FATAL,"Native near shadow cache store: %s",error.c_str());
                s_nearLastStoredDraws=s_nearStaticDraws;
            }
        }
        R_DrawNearKind(context,drawList,false);
    }
    else if ( partition->partitionIndex == 1 && cacheKey )
    {
        if ( !s_sunShadowFarRestored )
        {
            // Rebuild: static casters, keep them, then the dynamic ones on top.
            R_DrawSunShadowSurfsOfKind(context, drawList, false);
            std::string error;
            auto scope = bo1::vulkan::pass_recording::Current();
            if ( !scope || !scope->StoreSunShadowCache(context.state, R_SunShadowFarRect(data->data), cacheKey, error) )
                Com_Error(ERR_FATAL, "Native sun shadow cache store: %s", error.c_str());
        }
        R_DrawSunShadowSurfsOfKind(context, drawList, true);
    }
    else if(partition->partitionIndex==1&&!s_shadowQuality.farBrushes)
        R_DrawSunShadowSurfsOfKind(context,drawList,-1);
    else
        R_DrawSurfs(context, 0, drawList);
#else
    R_DrawSurfs(context, 0, drawList);
#endif
#if defined(__ANDROID__)
    if (report)
    {
        unsigned byType[16]{};
        for (unsigned i = 0; i < drawList->drawSurfCount; ++i)
            ++byType[drawList->drawSurfs[i].fields.surfType & 15];
        __android_log_print(ANDROID_LOG_INFO, "BO1NativeVulkan",
            "sun_shadow_partition index=%u restored=%d entries=%u draws=%u bsp=%u bsp_pretess=%u smodel=%u smodel_pretess=%u smodel_cached=%u smodel_skinned=%u bmodel=%u xmodel_rigid=%u xmodel_rigid_skinned=%u xmodel_skinned=%u other=%u",
            partition->partitionIndex,
#if defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
            (int)s_sunShadowFarRestored,
#else
            0,
#endif
            drawList->drawSurfCount,
            bo1::vulkan::material_dispatch::ReadProfile().draws - drawsBefore,
            byType[0], byType[1], byType[2], byType[3], byType[4], byType[5], byType[6], byType[7], byType[8], byType[9],
            byType[10] + byType[11] + byType[12] + byType[13] + byType[14] + byType[15]);
#if defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
        if(!partition->partitionIndex)
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
                "sun_near_cache enabled=%u ready=%u restored=%u key=%llu changed=%u eligible_entries=%u rejected_entries=%u static_draws=%u dynamic_draws=%u stored_static_draws=%u prepare_ms=%.3f serialized_bytes=%llu materials=%u definitions=%u decl_dedup=%u unique_decl=%u decl_bytes=%llu geometry_bytes=%llu",
                unsigned(s_nearEnabled),unsigned(s_nearDecision.ready),unsigned(s_nearRestored),(unsigned long long)s_nearDecision.key,
                unsigned(s_nearDecision.changed),s_nearEntries,s_nearRejected,s_nearStaticDraws,s_nearDynamicDraws,s_nearLastStoredDraws,
                s_nearEnabled?s_nearPrepareMs:0.0,(unsigned long long)(s_nearEnabled?s_nearSerializedBytes:0),
                s_nearEnabled?s_nearMaterialCount:0,s_nearEnabled?s_nearDefinitionCount:0,
                unsigned(s_nearEnabled&&s_nearDeclDedup),s_nearEnabled?s_nearUniqueDeclarations:0,
                (unsigned long long)(s_nearEnabled?s_nearDeclarationBytes:0),(unsigned long long)(s_nearEnabled?s_nearGeometryBytes:0));
        if(partition->partitionIndex==1)
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
                "sun_bmodel_cache enabled=%u original_key=%llu brushes=%u eligible=%u stable=%u invalidated=%u skipped_entries=%u key=%llu far_brush_shadows=%u omitted_brush_entries=%u",
                unsigned(s_brushCacheMode.Enabled()),baseCacheKey,s_brushCount,s_eligibleCount,unsigned(s_cacheBrushes),unsigned(s_brushInvalidated),s_brushSkips,(unsigned long long)cacheKey,
                unsigned(s_shadowQuality.farBrushes),s_farBrushOmitted);
#endif
    }
#endif
    R_HW_DisableScissor(context.state->prim.device);
}

