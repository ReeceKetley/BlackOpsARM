#include "r_draw_material.h"
#include "r_draw_method.h"
#include "r_char_tech_util.h"
#include "r_draw_shadowablelight.h"
#include "rb_pixelcost.h"
#include "r_shader_constant_set.h"
#include "r_dvars.h"
#include "rb_logfile.h"
#include "r_foliage.h"
#include "r_warn.h"
#include "rb_draw3d.h"
#include <universal/profile.h>
#include <qcommon/cm_mapkit.h>
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
#include "gfx_vulkan/forward_opaque.h"
#include "gfx_vulkan/shader_assets.h"
#include <android/log.h>
#include <atomic>
#include <unordered_map>
namespace bo1::vulkan::forward_opaque {
namespace {
std::atomic<bool> enabled{};
std::atomic<unsigned> diagnosticEpoch{};
thread_local const GfxCmdBufState* mainState{};
thread_local const GfxDrawSurfListInfo* prepassMain{};
thread_local bool mainSurfaceEligible{};
thread_local std::uint64_t omittedGroups{};
thread_local std::unordered_map<std::uint64_t,bool> fragmentProofs;
bool SurfaceEligible(GfxDrawSurf surf) {
    // Keep skinned actors, viewmodels, effects, water and special prepasses.
    return surf.fields.surfType<=SF_BMODEL&&surf.fields.surfType!=SF_STATICMODEL_SKINNED
        &&!R_GfxDrawSurf_GetFade(&surf)&&!TechLit_NeedsCharredTech(&surf)
        &&((surf.packed>>55)&3)==0;
}
bool MaterialEligible(const Material* material,unsigned type) {
    const MaterialTechnique* observedTechnique=nullptr;
    unsigned observedWords[2]{};
    auto result=[&](bool eligible,const char* reason) {
        struct Seen {const Material* material;unsigned type;};
        static thread_local Seen seen[16]{};
        static thread_local unsigned seenCount{};
        static thread_local unsigned seenEpoch{};
        const unsigned epoch=diagnosticEpoch.load(std::memory_order_relaxed);
        if(seenEpoch!=epoch){seenEpoch=epoch;seenCount=0;}
        if(Enabled()&&seenCount<16) {
            bool duplicate=false;
            for(unsigned i=0;i<seenCount;++i)if(seen[i].material==material&&seen[i].type==type)duplicate=true;
            if(!duplicate) {
                seen[seenCount++]={material,type};
                // Gather diagnostic metadata even when an earlier conservative gate rejected it.
                if(material&&type<115) {
                    observedTechnique=Material_GetTechnique(material,type);
                    const unsigned entry=material->stateBitsEntry[type];
                    if(material->stateBitsTable&&entry<material->stateBitsCount) {
                        observedWords[0]=material->stateBitsTable[entry].loadBits[0];
                        observedWords[1]=material->stateBitsTable[entry].loadBits[1];
                    }
                }
                __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
                    "forward_material material=%s type=%u words=%08x/%08x stateflag=%u techflag=%u passes=%u custom_sampler_flags=%u eligible=%d reason=%s",
                    material&&material->info.name?material->info.name:"<null>",type,observedWords[0],observedWords[1],
                    material?unsigned(material->stateFlags):0,observedTechnique?unsigned(observedTechnique->flags):0,
                    observedTechnique?unsigned(observedTechnique->passCount):0,
                    observedTechnique&&observedTechnique->passCount?unsigned(observedTechnique->passArray[0].customSamplerFlags):0,int(eligible),reason);
            }
        }
        return eligible;
    };
    if(!material||!((type>=10&&type<=37)||(type>=73&&type<=114))
        ||!(material->stateFlags&1)||!material->stateBitsTable)return result(false,"material_type_or_opaque_flag");
    const auto* technique=Material_GetTechnique(material,type);
    observedTechnique=technique;
    if(!technique||technique->passCount!=1||(technique->flags&0x73u))return result(false,"technique_pass_or_flags");
    const unsigned entry=material->stateBitsEntry[type];
    if(entry>=material->stateBitsCount)return result(false,"state_entry");
    const auto& bits=material->stateBitsTable[entry];
    observedWords[0]=bits.loadBits[0];observedWords[1]=bits.loadBits[1];
    if(!EligibleWords(bits.loadBits[0],bits.loadBits[1]))return result(false,"state_words");
    const auto& pass=technique->passArray[0];
    if(!SafeCustomSamplers(pass.customSamplerFlags))return result(false,"custom_sampler");
    const unsigned count=pass.perPrimArgCount+pass.perObjArgCount+pass.stableArgCount;
    if(count&&!pass.args)return result(false,"missing_args");
    for(unsigned i=0;i<count;++i)if(pass.args[i].type==4) {
        const auto source=pass.args[i].u.codeSampler;
        // Only immutable lighting inputs; no scene/FloatZ/post/feedback reads.
        if(!(source<=TEXTURE_SRC_CODE_SHADOWMAP_SPOT
            ||(source>=TEXTURE_SRC_CODE_LIGHT_ATTENUATION&&source<=TEXTURE_SRC_CODE_OUTDOOR)
            ||source==TEXTURE_SRC_CODE_REFLECTION_PROBE||source==TEXTURE_SRC_CODE_LIGHTMAP_SECONDARYB))return result(false,"code_sampler");
    }
    const auto pixel=shader_assets::Acquire(pass.pixelShader);
    if(!pixel||pixel->translation.stage!=ShaderStage::Fragment)return result(false,"pixel_asset");
    auto proof=fragmentProofs.find(pixel->serial);
    if(proof==fragmentProofs.end()) {
        if(fragmentProofs.size()>=4096)fragmentProofs.clear();
        // Registered records keep reflection only: translate the original
        // bytecode again for this one-time proof.
        bo1::vulkan::ShaderTranslation translated;
        const bool translatedOk=bo1::vulkan::TranslateShader(pixel->bytecode.data(),pixel->bytecode.size()*4,translated)
            &&translated.stage==ShaderStage::Fragment;
        proof=fragmentProofs.emplace(pixel->serial,translatedOk&&SafeFragment(translated.spirv.data(),translated.spirv.size())).first;
    }
    return result(proof->second,proof->second?"accepted":"fragment_side_effect");
}
}
void SetEnabled(bool value){
    if(enabled.exchange(value,std::memory_order_relaxed)!=value)
        diagnosticEpoch.fetch_add(1,std::memory_order_relaxed);
}
bool Enabled(){return enabled.load(std::memory_order_relaxed);}
bool ViewEligible(const GfxViewInfo* view) {
    const bool eligible=Enabled()&&shader_assets::Active()&&view&&!view->isMissileCamera&&!view->renderingFloatZ
        &&dx.supportsIntZ&&r_depthPrepass&&r_depthPrepass->current.enabled
        &&!r_reflectionProbeGenerate->current.enabled&&!view->drawList[6].drawSurfCount
        &&view->drawList[DRAWLIST_LIT].baseTechType==10&&!(view->sceneComposition.renderingMode&7)
        &&pixelCostMode==GFX_PIXEL_COST_MODE_OFF&&!r_showPenetration->current.integer;
    static thread_local unsigned checks{};
    static thread_local unsigned checksEpoch{};
    const unsigned epoch=diagnosticEpoch.load(std::memory_order_relaxed);
    if(checksEpoch!=epoch){checksEpoch=epoch;checks=0;}
    if(Enabled()&&(++checks==1||checks%500==0))
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
            "forward_view checks=%u eligible=%d active=%d view=%d intz=%d floatz=%d missile=%d depthprepass=%d reflection=%d cloak_count=%u base_tech=%u composition=%u pixelcost=%d penetration=%d",
            checks,int(eligible),int(shader_assets::Active()),int(view!=nullptr),int(dx.supportsIntZ),
            view?int(view->renderingFloatZ):-1,view?int(view->isMissileCamera):-1,
            r_depthPrepass?int(r_depthPrepass->current.enabled):-1,
            r_reflectionProbeGenerate?int(r_reflectionProbeGenerate->current.enabled):-1,
            view?unsigned(view->drawList[6].drawSurfCount):0,view?unsigned(view->drawList[DRAWLIST_LIT].baseTechType):0,
            view?unsigned(view->sceneComposition.renderingMode):0,int(pixelCostMode),
            r_showPenetration?r_showPenetration->current.integer:-1);
    return eligible;
}
MainLitScope::MainLitScope(const GfxCmdBufState* state,bool active):previous_(mainState){mainState=active?state:nullptr;mainSurfaceEligible=false;}
MainLitScope::~MainLitScope(){mainState=previous_;mainSurfaceEligible=false;}
PrepassScope::PrepassScope(const GfxDrawSurfListInfo* list,bool active):previous_(prepassMain){prepassMain=active?list:nullptr;}
PrepassScope::~PrepassScope(){prepassMain=previous_;}
std::uint64_t OmittedGroups(){return omittedGroups;}
bool Apply(const GfxCmdBufState* state,unsigned words[2]) {
    if(!mainState||!mainSurfaceEligible||!state||state->renderTargetId!=mainState->renderTargetId
        ||state->depthRangeType!=GFX_DEPTH_RANGE_SCENE||state->passIndex!=0
        ||!MaterialEligible(state->material,state->techType))return false;
    return ConvertWords(words);
}
bool OmitPrepass(GfxCmdBufContext context,GfxDrawSurf surf) {
    if(!prepassMain||!SurfaceEligible(surf))return false;
    const auto* material=rgp.sortedMaterials[surf.fields.materialSortedIndex];
    if(!material||(surf.fields.surfType==SF_TRIANGLES&&(material->info.gameFlags&8)))return false;
    const auto type=R_GetTechType(context,prepassMain,prepassMain->baseTechType,surf);
    unsigned char candidates[4]{};R_GetPossibleTechniqueTypes(type,surf,candidates);
    for(unsigned i=0;i<4&&candidates[i]!=255;++i)if(Material_GetTechnique(material,candidates[i])) {
        if(!MaterialEligible(material,candidates[i]))return false;
        ++omittedGroups;return true;
    }
    return false;
}
void MainSurface(GfxDrawSurf surf){if(mainState)mainSurfaceEligible=SurfaceEligible(surf);}
}
#endif

unsigned __int8 gNvFloatZTech_Intz[70] =
{
  80u,
  81u,
  82u,
  83u,
  84u,
  85u,
  86u,
  87u,
  88u,
  89u,
  90u,
  91u,
  92u,
  93u,
  94u,
  95u,
  96u,
  97u,
  98u,
  99u,
  100u,
  101u,
  102u,
  103u,
  104u,
  105u,
  106u,
  107u,
  38u,
  39u,
  40u,
  41u,
  42u,
  43u,
  44u,
  45u,
  46u,
  47u,
  48u,
  49u,
  50u,
  51u,
  52u,
  53u,
  54u,
  55u,
  56u,
  57u,
  58u,
  59u,
  59u,
  61u,
  61u,
  63u,
  63u,
  65u,
  65u,
  67u,
  67u,
  69u,
  69u,
  71u,
  71u,
  108u,
  109u,
  110u,
  111u,
  112u,
  113u,
  114u
};

unsigned __int8 __cdecl RemoveSunShadowTech(unsigned __int8 srcTech)
{
    unsigned __int8 remappedTech; // [esp+7h] [ebp-1h]

    switch ( srcTech )
    {
        case 0xCu:
            remappedTech = 11;
            break;
        case 0x28u:
            remappedTech = 39;
            break;
        case 0x2Fu:
            remappedTech = 46;
            break;
        case 0x36u:
            remappedTech = 53;
            break;
        case 0x3Fu:
            remappedTech = 61;
            break;
        case 0x40u:
            remappedTech = 62;
            break;
        case 0x4Bu:
            remappedTech = 74;
            break;
        default:
            remappedTech = srcTech;
            break;
    }
    return remappedTech;
}

unsigned int __cdecl R_GfxDrawSurf_GetFade(const GfxDrawSurf *drawSurf)
{
    if (drawSurf->fields.surfType == 2)
    {
        return drawSurf->fields.fade;
    }

    return 0;
}

unsigned __int8 __cdecl R_Get_NvFloatZLitTech(const Material *material, unsigned __int8 originalTechType)
{
    unsigned __int8 newTechType; // [esp+Bh] [ebp-9h]
    unsigned int index; // [esp+Ch] [ebp-8h]

    if ( !material )
        return originalTechType;
    if ( (Material_GetTechniqueSet(material)->techsetFlags & 1) == 0 )
        return originalTechType;
    index = originalTechType - 10;
    if ( index >= 0x46 )
        return originalTechType;
    newTechType = gNvFloatZTech_Intz[index];
    if ( Material_GetTechnique(material, newTechType) )
        return newTechType;
    else
        return originalTechType;
}

unsigned __int8 __cdecl R_GetTechType(
                GfxCmdBufContext context,
                const GfxDrawSurfListInfo *info,
                unsigned __int8 baseTechType,
                GfxDrawSurf drawSurf)
{
    unsigned __int8 result; // al
    GfxFadeType fadeType; // [esp+34h] [ebp-18h]
    const GfxBackEndData *data; // [esp+40h] [ebp-Ch]
    char needsCharredTech; // [esp+46h] [ebp-6h]
    unsigned __int8 actualTechType; // [esp+47h] [ebp-5h]
    unsigned int surfType; // [esp+48h] [ebp-4h]

    surfType = drawSurf.fields.surfType;

    switch ( baseTechType )
    {
        case 5u:
        case 6u:
            if ( !dx.supportsIntZ || context.state->renderTargetId == 22 )
                goto LABEL_38;
            if ( baseTechType == 5 )
                return 7;
            if ( baseTechType == 6 )
                result = 8;
            else
LABEL_38:
                result = baseTechType;
            break;
        case 0xAu:
            // mod: mapkit-fix-4g - empty-base brush models take the retail lit technique again (8e50396's unlit
            // technique drew the kit boards at 4x retail brightness); R_TessBModel binds them a neutral lightmap.
            needsCharredTech = TechLit_NeedsCharredTech(&drawSurf);
            fadeType = (GfxFadeType)(R_GfxDrawSurf_GetFade(&drawSurf) != 0);
            data = context.source->input.data;

            bcassert(drawSurf.fields.primaryLightIndex, data->shadowableLightCount);
            
            actualTechType = data->primaryLightTechType[surfType][fadeType][needsCharredTech != 0][drawSurf.fields.primaryLightIndex];
            if ( info->isMissileCamera || (drawSurf.packed & 0x200000000000000LL) != 0 )
                actualTechType = RemoveSunShadowTech(actualTechType);
            if ( actualTechType >= 0xAu && actualTechType <= 0x10u )
            {
                if ( ((drawSurf.packed >> 29) & 1) != 0 && ((drawSurf.packed >> 30) & 1) != 0 )
                {
                    actualTechType += 21;
                }
                else if ( ((drawSurf.packed >> 29) & 1) != 0 )
                {
                    actualTechType += 14;
                }
                else if ( ((drawSurf.packed >> 30) & 1) != 0 )
                {
                    actualTechType += 7;
                }
            }
            if ( dx.supportsIntZ && context.state->renderTargetId != 22 )
                actualTechType = R_Get_NvFloatZLitTech(rgp.sortedMaterials[drawSurf.fields.materialSortedIndex], actualTechType);
            result = actualTechType;
            break;
        case 0x73u:
        case 0x74u:
        case 0x75u:
            if ( TechLit_NeedsCharredTech(&drawSurf) )
            {
                if ( baseTechType == 115 )
                {
                    result = 118;
                }
                else if ( baseTechType == 116 )
                {
                    result = 119;
                }
                else
                {
                    result = 120;
                }
            }
            else
            {
                result = baseTechType;
            }
            break;
        case 0x7Fu:
            if ( surfType == 4 || surfType == 3 )
                result = 0x80;
            else
                result = baseTechType;
            break;
        default:
            result = baseTechType;
            break;
    }
    return result;
}

int __cdecl R_GetPossibleTechniqueTypes(unsigned __int8 techType, GfxDrawSurf drawSurf, unsigned __int8 *techTypes)
{
    unsigned __int8 *tech; // [esp+20h] [ebp-8h]

    *techTypes = techType;
    tech = techTypes + 1;
    if ( techType >= 0xAu && techType < 0x73u && techType > 0x10u && techType <= 0x25u )
    {
        if ( ((drawSurf.packed >> 29) & 1) != 0 && ((drawSurf.packed >> 30) & 1) != 0 )
        {
            *tech = techType - 21;
            tech = techTypes + 2;
        }
        else if ( ((drawSurf.packed >> 30) & 1) != 0 )
        {
            *tech = techType - 7;
            tech = techTypes + 2;
        }
        else if ( ((drawSurf.packed >> 29) & 1) != 0 )
        {
            *tech = techType - 14;
            tech = techTypes + 2;
        }
    }
    *tech = -1;
    if ( (unsigned int)(tech + 1 - techTypes) > 4
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_draw_material.cpp",
                    738,
                    0,
                    "%s",
                    "count <= (uint)MAX_POSSIBLE_TECHNIQUES") )
    {
        __debugbreak();
    }
    return tech + 1 - techTypes;
}

int __cdecl R_SetTechnique(
                GfxCmdBufContext context,
                GfxCmdBufContext *prepassContext,
                const GfxDrawSurfListInfo *info,
                GfxDrawSurf drawSurf)
{
    PROF_SCOPED("R_SetTechnique");
#if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    bo1::vulkan::forward_opaque::MainSurface(drawSurf);
    if(info->baseTechType==0&&bo1::vulkan::forward_opaque::OmitPrepass(context,drawSurf))return 0;
#endif

    unsigned __int64 v5; // rax
    GfxDrawSurf overrideDrawSurf; // [esp+8h] [ebp-10h]
    unsigned __int8 techType; // [esp+13h] [ebp-5h]
    unsigned __int8 techTypes[4]; // [esp+14h] [ebp-4h] BYREF

    techType = R_GetTechType(context, info, info->baseTechType, drawSurf);
    R_GetPossibleTechniqueTypes(techType, drawSurf, techTypes);
    if ( techType > 1u )
    {
        if ( info->baseTechType == 10 )
            R_SetShadowableLight(context.source, drawSurf.fields.primaryLightIndex);
        if ( !R_TrySetMaterialWithFunc(
                        R_SetMaterial,
                        context,
                        drawSurf,
                        techTypes) )
            return 0;
    }
    else if ( !R_TrySetMaterialWithFunc(
                             R_SetPrepassMaterial,
                             context,
                             drawSurf,
                             techTypes) )
    {
        return 0;
    }
    if ( prepassContext->state && !R_SetPrepassMaterial(*prepassContext, drawSurf, 0) )
        prepassContext->state = 0;
    context.state->origMaterial = context.state->material;
    context.state->origTechType = context.state->techType;
    if ( pixelCostMode > GFX_PIXEL_COST_MODE_MEASURE_MSEC )
    {
        //v5 = ((R_PixelCost_GetAccumulationMaterial(context.state->material)->info.drawSurf.packed >> 31) & 0xFFF) << 31;
        //*(unsigned int *)&overrideDrawSurf.fields = v5 | *(unsigned int *)&drawSurf.fields & 0x7FFFFFFF;
        //HIDWORD(overrideDrawSurf.packed) = HIDWORD(v5) | HIDWORD(drawSurf.packed) & 0xFFFFF800;
        overrideDrawSurf = drawSurf;
        overrideDrawSurf.fields.materialSortedIndex = R_PixelCost_GetAccumulationMaterial(context.state->material)->info.drawSurf.fields.materialSortedIndex;

        R_SetMaterial(context, overrideDrawSurf, 4u);
    }
    return 1;
}

int __cdecl R_SetPrepassMaterial(GfxCmdBufContext context, GfxDrawSurf drawSurf, unsigned __int8 techType)
{
    const MaterialTechnique *technique; // [esp+8h] [ebp-Ch]
    Material *material; // [esp+Ch] [ebp-8h]
    unsigned int prepass; // [esp+10h] [ebp-4h]

    prepass = (drawSurf.packed >> 55) & 3;
    if ( prepass == 2 )
        return 0;
    if ( R_GfxDrawSurf_GetFade(&drawSurf) )
        return 0;
    material = rgp.sortedMaterials[drawSurf.fields.materialSortedIndex];
    if ( !prepass && (material->stateFlags & 1) != 0 )
        material = rgp.depthPrepassMaterial;
    context.state->material = material;
    technique = Material_GetTechnique(material, techType);
    context.state->technique = technique;
    if ( !technique )
        return 0;
    if ( (technique->flags & 2) != 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_draw_material.cpp",
                    214,
                    0,
                    "%s",
                    "!(technique->flags & MTL_TECHFLAG_NEEDS_RESOLVED_SCENE)") )
    {
        __debugbreak();
    }
    if ( (technique->flags & 1) != 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_draw_material.cpp",
                    215,
                    0,
                    "%s",
                    "!(technique->flags & MTL_TECHFLAG_NEEDS_RESOLVED_POST_SUN)") )
    {
        __debugbreak();
    }
    if ( !material
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_draw_material.cpp", 217, 0, "%s", "material") )
    {
        __debugbreak();
    }
    RB_ApplyShaderConstantSet(context.source, &drawSurf);
    context.state->techType = techType;
    return 1;
}

int __cdecl R_SetMaterial(GfxCmdBufContext context, GfxDrawSurf drawSurf, unsigned __int8 techType)
{
    const char *v4; // eax
    const char *v5; // eax
    float v6; // [esp+10h] [ebp-34h]
    float v8; // [esp+24h] [ebp-20h]
    float fadeVal; // [esp+30h] [ebp-14h]
    const MaterialTechnique *technique; // [esp+38h] [ebp-Ch]
    int penetrationMode; // [esp+3Ch] [ebp-8h]
    const Material *material; // [esp+40h] [ebp-4h]

    while ( 1 )
    {
        material = rgp.sortedMaterials[drawSurf.fields.materialSortedIndex];
        technique = Material_GetTechnique(material, techType);
        context.state->material = material;
        context.state->technique = technique;
        if ( !technique )
            return 0;
        penetrationMode = r_showPenetration->current.integer;
        if ( !penetrationMode )
            break;
        if ( !techType || techType == 1 || techType == 2 || techType == 3 )
            break;
        if ( techType == 4 )
            break;
        if ( ((material->info.gameFlags & 0x80) == 0) != (penetrationMode == 1) )
            break;
        v8 = context.source->materialTime * 5.0;
        v6 = floor(v8);
        if ( v8 - v6 >= 0.5 )
            break;
        techType = 4;
    }
    if ( (technique->flags & 2) != 0
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_draw_material.cpp",
                    360,
                    0,
                    "%s",
                    "!(technique->flags & MTL_TECHFLAG_NEEDS_RESOLVED_SCENE)") )
    {
        __debugbreak();
    }
    if ( (techType == 5 || techType == 4) && (technique->flags & 0x10) != 0 && !context.source->constVersions[28] )
        return 0;
    if ( !material
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_draw_material.cpp", 370, 0, "%s", "material") )
    {
        __debugbreak();
    }
    if ( r_logFile->current.integer )
    {
        v4 = RB_LogTechniqueType(techType);
        v5 = va("R_SetMaterial( %s, %s, %s )\n", material->info.name, technique->name, v4);
        RB_LogPrint(v5);
    }
    if ( R_GfxDrawSurf_GetFade(&drawSurf) )
    {
        fadeVal = (float)((drawSurf.packed >> 16) & 0xF) * 0.0625;
        context.source->input.consts[33][0] = fadeVal;
        context.source->input.consts[33][1] = fadeVal;
        context.source->input.consts[33][2] = fadeVal;
        context.source->input.consts[33][3] = fadeVal;
        R_DirtyCodeConstant(context.source, CONST_SRC_CODE_ALPHA_FADE);
    }
    RB_ApplyShaderConstantSet(context.source, &drawSurf);
    if ( drawSurf.fields.surfType == 0 && (material->info.gameFlags & 8) != 0 )
        return 0;
    context.state->techType = techType;
    return 1;
}

int __cdecl R_TrySetMaterialWithFunc(
                int (__cdecl *func)(GfxCmdBufContext, GfxDrawSurf, const unsigned __int8),
                GfxCmdBufContext context,
                GfxDrawSurf drawSurf,
                unsigned __int8 *techType)
{
    const char *v5; // [esp+0h] [ebp-18h]
    unsigned int i; // [esp+4h] [ebp-14h]
    int ret; // [esp+8h] [ebp-10h]
    unsigned __int8 techAssigned; // [esp+Fh] [ebp-9h]
    const char *techName; // [esp+10h] [ebp-8h]

    ret = 0;
    techAssigned = -1;
    for ( i = 0; techType[i] != 255; ++i )
    {
        if (func(context, drawSurf, techType[i] ))
        {
            if ( !i )
                return 1;
            techAssigned = techType[i];
            ret = 1;
            break;
        }
    }
#if defined(__ANDROID__)
    // These are intentional exclusions in R_SetPrepassMaterial, not missing
    // techniques. Preserve the skipped draw without reporting a shader error.
    if (func == R_SetPrepassMaterial && !ret
        && (((drawSurf.packed >> 55) & 3) == 2 || R_GfxDrawSurf_GetFade(&drawSurf)))
        return 0;
#endif
    techName = RB_LogTechniqueType(*techType);
    if ( techAssigned == 255 )
        v5 = "TECHNIQUE_NONE";
    else
        v5 = RB_LogTechniqueType(techAssigned);
    R_WarnOncePerFrame(
        R_WARN_MISSING_TECHNIQUE,
        rgp.sortedMaterials[drawSurf.fields.materialSortedIndex]->info.name,
        techName,
        *techType,
        v5,
        techAssigned);
    return ret;
}

void __cdecl R_SetGameTime(GfxCmdBufSourceState *source, float gameTime)
{
    float gameTimeVec[4]; // [esp+1Ch] [ebp-10h] BYREF

    R_CalcGameTimeVec(gameTime, gameTimeVec);

    R_SetCodeConstant(source,
        CONST_SRC_CODE_GAMETIME,
        gameTimeVec[0],
        gameTimeVec[1],
        gameTimeVec[2],
        gameTimeVec[3]
        );
}

int __cdecl R_UpdateMaterialTime(
                GfxCmdBufSourceState *source,
                float materialTime,
                float burn,
                float fade,
                float wetness)
{
    source->destructibleBurnAmount = burn;
    source->destructibleFadeAmount = fade;
    source->wetness = wetness;
    if (materialTime == source->materialTime)
        return 0;
    source->materialTime = materialTime;
    R_SetGameTime(source, source->sceneDef.floatTime - materialTime);
    return 1;
}

