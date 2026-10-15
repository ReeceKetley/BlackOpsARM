#include "native_vulkan_material_preview.h"
#include "post_fusion_trial_bytecode.h"
#include "gfx_d3d/r_material.h"
#include "gfx_d3d/r_image.h"
#include "gfx_d3d/r_image_load_common.h"
#include "gfx_d3d/r_state.h"
#include "gfx_vulkan/target_bindings.h"
#include "gfx_d3d/r_init.h"
#include "gfx_d3d/rb_sky.h"
#include "gfx_d3d/rb_corona.h"
#include "gfx_d3d/rb_superflare.h"
#include "client/cl_compositing.h"
#include "gfx_d3d/r_rendertarget.h"
#include "gfx_d3d/r_state_utils.h"
#include "gfx_d3d/rb_draw3d.h"
#include "gfx_d3d/r_shade.h"
#include "gfx_d3d/r_buffers.h"
#include "gfx_d3d/r_water_sim.h"
#include "gfx_d3d/r_rendercmds.h"
#include "gfx_d3d/r_meshdata.h"
#include "gfx_d3d/r_staticmodelcache.h"
#include "gfx_d3d/r_draw_staticmodel.h"
#include "gfx_d3d/r_bsp.h"
#include "gfx_d3d/r_bsp_load_obj.h"
#include "gfx_d3d/r_vertexstream2.h"
#include "gfx_d3d/r_xsurface_optimize.h"
#include "gfx_d3d/r_draw_model_util.h"
#include "gfx_d3d/rb_tess.h"
#include "gfx_d3d/r_dvars.h"
#include "gfx_vulkan/hardware_info.h"
#include "gfx_vulkan/material_state.h"
#include "gfx_d3d/rb_backend.h"
#include <cstdio>
#include <cstring>
#include <android/log.h>
extern IDirect3DQuery9* SuperFlareQuery[5][2][4][4];
namespace bo1::android {
namespace {
bool Check(VkResult result,const char* action,std::string& error) {if(result==VK_SUCCESS)return true;error=std::string(action)+": "+std::to_string(int(result));return false;}
}
namespace {
bool CheckMeshUploads(std::string& error) {
    using namespace bo1::vulkan;
    GfxMeshData quad{};R_InitDynamicMesh(&quad,6,4,32);
    struct QuadCleanup {GfxMeshData* mesh;~QuadCleanup(){R_ShutdownDynamicMesh(mesh);}} quadCleanup{&quad};
    R_SetQuadMeshData(&quad,10,20,30,40,0,0,1,1,0xffffffff);
    auto first=buffer_assets::Acquire(&quad.vb.buffer);GfxVertex previous[4]{};
    if(!first||first->payloadBytes!=128||quad.vb.verts||quad.indexCount!=6
        ||!first->gpu->Read(0,previous,sizeof(previous),error)||previous[0].xyzw[0]!=10) {error="Native quad mesh publication failed";return false;}
    R_SetQuadMeshData(&quad,20,20,30,40,0,0,1,1,0xffffffff);
    auto next=buffer_assets::Acquire(&quad.vb.buffer);GfxVertex retained[4]{};
    if(!next||next->gpu==first->gpu||!first->gpu->Read(0,retained,sizeof(retained),error)
        ||std::memcmp(previous,retained,sizeof(previous))) {error="Native quad overwrite changed retained buffer";return false;}
    auto backend=std::make_unique<GfxBackEndData>();GfxVertexBufferState skin{};GfxMeshData code{};GfxIndexBufferState indices{};
    unsigned frame=7;GfxVertex vertices[3]{};unsigned short triangle[3]={0,1,2};
    vertices[0].xyzw[0]=10;
    backend->skinnedCacheVb=&skin;backend->codeMeshPtr=&code;backend->preTessIb=&indices;backend->dynamicBufferCurrentFrame=&frame;
    std::array<const void*,5> owners={&skin.buffer,&code.vb.buffer,&backend->markMesh.vb.buffer,&backend->glassMesh.vb.buffer,&indices.buffer};
    struct Cleanup {std::array<const void*,5>& owners;~Cleanup(){for(auto owner:owners)buffer_assets::Release(owner);}} cleanup{owners};
    for(auto vb:{&skin,&code.vb,&backend->markMesh.vb,&backend->glassMesh.vb}) {
        R_InitDynamicVertexBufferState(vb,sizeof(vertices));vb->verts=reinterpret_cast<unsigned char*>(vertices);vb->used=sizeof(vertices);
    }
    R_InitDynamicIndexBufferState(&indices,3);indices.indices=triangle;indices.used=3;
    RB_UpdateDynamicBuffers(backend.get());
    for(unsigned i=0;i<owners.size();++i) {
        auto record=buffer_assets::Acquire(owners[i]);
        if(!record||record->payloadBytes!=(i==4?sizeof(triangle):sizeof(vertices))) {error="Native bulk upload publication failed";return false;}
    }
    if(frame) {error="Native bulk upload did not release staging frame";return false;}
    auto oldSkin=buffer_assets::Acquire(&skin.buffer);vertices[0].xyzw[0]=20;frame=8;
    RB_UpdateDynamicBuffers(backend.get());GfxVertex oldVertices[3]{};
    if(frame||oldSkin->gpu==buffer_assets::Acquire(&skin.buffer)->gpu
        ||!oldSkin->gpu->Read(0,oldVertices,sizeof(oldVertices),error)||oldVertices[0].xyzw[0]!=10) {error="Native bulk discard changed retained skin buffer";return false;}
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_quad_mesh=1 mapped_discard_retains_previous=1 bulk_skin_mark_code_glass_pretess=1 staging_frame_released=1");return true;
}
}
NativeMaterialPreview::NativeMaterialPreview(bo1::vulkan::Presentation& p,const char* root):presentation_(p),root_(root){}
NativeMaterialPreview::~NativeMaterialPreview() {
    // Presentation still owns the live device. Retire draws before members
    // release pipeline, descriptors, images and persistently mapped buffers.
    vkDeviceWaitIdle(presentation_.Device());program_.reset();
    eventRecording_.reset();std::string eventError;
    for(auto& arena:eventFrames_)if(!arena.Retire(eventError))std::abort();
    if(eventAllocated_&&!bo1::vulkan::event_fences::Clear(eventError))std::abort();
    queryRecording_.reset();if(engineQueryAllocated_)RB_HW_ReleaseOcclusionQueryFromOwner(&engineQuery_);
    for(auto& arena:queryFrames_)arena.Retire();
    for(auto pool:pools_)if(pool)vkDestroyDescriptorPool(presentation_.Device(),pool,nullptr);
    bo1::vulkan::shader_assets::Release(&vertexShader_);bo1::vulkan::shader_assets::Release(&pixelShader_);
    bo1::vulkan::vertex_assets::Release(&declaration_);
    if(image_)bo1::vulkan::image_assets::Release(image_.get());
    bo1::vulkan::texture_bindings::Release(commandState_.get());textureBindings_.reset();
    for(auto& program:statePrograms_)program.reset();if(statePool_)vkDestroyDescriptorPool(presentation_.Device(),statePool_,nullptr);
    generatedProgram_.reset();if(generatedPool_)vkDestroyDescriptorPool(presentation_.Device(),generatedPool_,nullptr);
    generatedBindings_.reset();generatedSourcePass_.reset();generatedOutputPass_.reset();generatedSource_.reset();generatedOutput_.reset();
    if(programCache_)for(auto& arena:passFrames_)arena.Retire(*programCache_);
    generatedSharedDepth_.reset();
    for(auto& frame:dispatchFrames_)frame.clear();
    programCache_.reset();
    occlusion_.reset();
    probePass_.reset();scratchPass_.reset();loadPass_.reset();stencilPass_.reset();depthOnlyPass_.reset();stencilDepth_.reset();probeTarget_.reset();scratchDepth_.reset();probeReadback_.reset();
    if(commandState_) {bo1::vulkan::geometry_bindings::Release(&commandState_->prim);bo1::vulkan::constant_state::Release(commandState_.get());}
    constantFrames_={};drawFrames_={};
    if(commandState_)bo1::vulkan::material_bindings::Release(commandState_.get());
    for(auto& arena:geometryFrames_)arena.clear();
    if(geometry_) {bo1::vulkan::buffer_assets::Release(&geometry_->indexBuffer);bo1::vulkan::buffer_assets::Release(&geometry_->vb0);}
    vertexBufferAsset_.reset();indexBufferAsset_.reset();
    imageAsset_.reset();texture_.reset();
    std::string detachError;
    if(!bo1::vulkan::image_assets::Attach(VK_NULL_HANDLE,VK_NULL_HANDLE,detachError)
        ||!bo1::vulkan::shader_assets::SelectNative(false))
        __android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","native_image_detach_failed=%s",detachError.c_str());
}
void NativeMaterialPreview::Submitted(void* self,unsigned slot,VkFence fence) {
    auto& owner=*static_cast<NativeMaterialPreview*>(self);
    if(owner.eventRecording_) {
        if(!owner.eventRecording_->Submitted(owner.error_))return;
        owner.eventRecording_.reset();
    }
    if(owner.queryRecording_) {
        if(!owner.queryRecording_->Submitted(fence,owner.error_))return;
        owner.queryRecording_.reset();
    }
    if(owner.queriesPending_&&slot==owner.probeSlot_) {
        if(!owner.occlusion_->Submitted(fence,owner.error_))return;
        owner.queriesPending_=false;
    }
}
void NativeMaterialPreview::Prepare(void* self,const bo1::vulkan::FrameContext& frame) {
    auto& owner=*static_cast<NativeMaterialPreview*>(self);if(owner.error_.empty())owner.Prepare(frame);
}
bool NativeMaterialPreview::Prepare(const bo1::vulkan::FrameContext& frame) {
    using namespace bo1::vulkan;const auto device=presentation_.Device();const auto physical=presentation_.Physical();
    if(!eventFrames_[frame.slot].Retire(error_))return false;
    if(!eventAllocated_) {R_InitNativeEventQueries();eventAllocated_=true;}
    eventRecording_=std::make_unique<event_fences::RecordingScope>(eventFrames_[frame.slot]);
    queryFrames_[frame.slot].Retire();
    if(!engineQueryAllocated_) {
        RB_HW_AllocOcclusionQueryFromOwner(&engineQuery_);engineQueryAllocated_=true;
        const auto oldViews=gfxCfg.maxClientViews;gfxCfg.maxClientViews=4;
        RB_AllocSunSpriteQueries();Corona corona{};RB_AllocCoronaSpriteQuery(&corona);RB_AllocSuperFlareQueries();
        for(unsigned view=0;view<4;++view)for(unsigned query=0;query<2;++query)
            if(sunFlareArray[view].sunQuery[query]||!RB_HW_HasOcclusionQueryFromOwner(&sunFlareArray[view].sunQuery[query])) {error_="Canonical native sun query allocation missing";return false;}
        for(unsigned query=0;query<5;++query)if(corona.query[query]||corona.queryMax[query]
            ||!RB_HW_HasOcclusionQueryFromOwner(&corona.query[query])||!RB_HW_HasOcclusionQueryFromOwner(&corona.queryMax[query])) {error_="Canonical native corona query allocation missing";return false;}
        unsigned flareCount=0;
        for(auto& frame:SuperFlareQuery)for(auto& type:frame)for(auto& client:type)for(auto& query:client) {
            if(query||!RB_HW_HasOcclusionQueryFromOwner(&query)) {error_="Canonical native superflare query allocation missing";return false;}
            ++flareCount;
        }
        RB_FreeSunSpriteQueries();RB_FreeCoronaSpriteQuery(&corona);RB_FreeSuperFlareQueries();gfxCfg.maxClientViews=oldViews;
        for(auto& frame:SuperFlareQuery)for(auto& type:frame)for(auto& client:type)for(auto& query:client)
            if(RB_HW_HasOcclusionQueryFromOwner(&query)) {error_="Canonical native superflare query release failed";return false;}

        for(unsigned view=0;view<4;++view)for(unsigned query=0;query<2;++query)if(RB_HW_HasOcclusionQueryFromOwner(&sunFlareArray[view].sunQuery[query])) {error_="Canonical native sun query release failed";return false;}
        for(unsigned query=0;query<5;++query)if(RB_HW_HasOcclusionQueryFromOwner(&corona.query[query])||RB_HW_HasOcclusionQueryFromOwner(&corona.queryMax[query])) {error_="Canonical native corona query release failed";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_query_family_alloc_release=1 sun8=1 corona10=1 superflare_count=%u no_COM_handles=1",flareCount);

        if(engineQuery_||!RB_HW_HasOcclusionQueryFromOwner(&engineQuery_)) {error_="Native engine query owner did not allocate";return false;}
    }
    queryRecording_=std::make_unique<query_assets::RecordingScope>(frame.command,queryFrames_[frame.slot]);
    if(!queryRecording_->Prepare(error_))return false;
    if(programCache_)passFrames_[frame.slot].Retire(*programCache_);
    dispatchFrames_[frame.slot].clear();
    geometryFrames_[frame.slot].clear();constantFrames_[frame.slot].reset();drawFrames_[frame.slot].reset();
    if(!texture_) {
        using namespace post_fusion_trial::bytecode;
        if(!image_assets::Attach(physical,device,error_))return false;
        if(!shader_assets::SelectNative(true)) {error_="Cannot select native shader assets while other owners remain";return false;}
        HardwareInfo hardware;if(!QueryHardware(physical,hardware,error_))return false;
        dvar_s targetLoad{},targetFastFile{};targetLoad.current.enabled=targetFastFile.current.enabled=true;
        const auto oldTargetLoad=r_loadForRenderer,oldTargetFastFile=useFastFile;r_loadForRenderer=&targetLoad;useFastFile=&targetFastFile;
        const auto oldWidth=vidConfig.sceneWidth,oldHeight=vidConfig.sceneHeight;vidConfig.sceneWidth=64;vidConfig.sceneHeight=32;
        const auto oldDisplayWidth=vidConfig.displayWidth,oldDisplayHeight=vidConfig.displayHeight;
        vidConfig.displayWidth=64;vidConfig.displayHeight=32;
        const auto oldMinimal=g_allocateMinimalResources;g_allocateMinimalResources=false;
        const auto oldReflection=r_reflectionProbeGenerate;dvar_s reflection{};r_reflectionProbeGenerate=&reflection;
        const auto frameFormat=R_InitFrameBufferRenderTarget();
        r_reflectionProbeGenerate=oldReflection;g_allocateMinimalResources=oldMinimal;
        vidConfig.displayWidth=oldDisplayWidth;vidConfig.displayHeight=oldDisplayHeight;
        const auto oldAllowNull=r_allow_null_rt;dvar_s allowNull{};allowNull.current.enabled=true;r_allow_null_rt=&allowNull;
        R_InitSceneNullRenderTarget_PC();r_allow_null_rt=oldAllowNull;
        auto depthScene=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_SCENE_NULLCOLOR]);
        auto canonicalFrame=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_FRAME_BUFFER]);
        auto canonicalScene=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_SCENE]);
        auto resolved=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_RESOLVED_SCENE]);
        if(frameFormat!=D3DFMT_A8R8G8B8||!canonicalFrame||canonicalScene!=canonicalFrame||!resolved
            ||canonicalFrame->color==resolved->color||canonicalFrame->depth!=resolved->depth) {error_="Canonical native framebuffer/scene initialization failed";return false;}
        if(!depthScene||depthScene->color||depthScene->depth!=canonicalFrame->depth||!dx.supportsSceneNullRenderTarget) {error_="Native depth-only scene target failed";return false;}
        AttachmentPass depthScenePass(device);if(!depthScenePass.Create(nullptr,depthScene->depth->texture,true,error_))return false;
        AttachmentPass framePass(device);
        if(!framePass.Create(canonicalFrame->color->texture,canonicalFrame->depth->texture,true,error_))return false;
        R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_SCENE_NULLCOLOR]);
        R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_FRAME_BUFFER]);
        if(image_assets::Acquire(gfxRenderTargets[R_RENDERTARGET_SCENE].image)!=canonicalFrame->color) {error_="Native framebuffer source release lost scene image";return false;}
        R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_SCENE]);
        R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_RESOLVED_SCENE]);
        render_targets::Clear();
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_framebuffer_scene_init=1 resolved_scene_separate_color=1 real_framebuffer_created=1 window_present_integrated=0");

        R_InitFullscreenRenderTargetImage(5,FULLSCREEN_SCENE,0,D3DFMT_A16B16G16R16F,RENDERTARGET_USAGE_RENDER,&gfxRenderTargets[R_RENDERTARGET_PINGPONG_0]);
        R_InitFullscreenRenderTargetImage(6,FULLSCREEN_SCENE,0,D3DFMT_A8R8G8B8,RENDERTARGET_USAGE_RENDER,&gfxRenderTargets[R_RENDERTARGET_PINGPONG_1]);
        vidConfig.sceneWidth=oldWidth;vidConfig.sceneHeight=oldHeight;
        auto firstTarget=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_0]),secondTarget=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_1]);
        if(!firstTarget||!secondTarget||!firstTarget->depth||firstTarget->depth!=secondTarget->depth
            ||firstTarget->color->description.format!=VK_FORMAT_R16G16B16A16_SFLOAT) {error_="Native fullscreen targets/shared depth failed";return false;}
        R_ShareRenderTarget(R_RENDERTARGET_PINGPONG_0,R_RENDERTARGET_RESOLVED_SCENE);
        auto sharedTarget=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_RESOLVED_SCENE]);
        if(sharedTarget!=firstTarget) {error_="Canonical native target sharing lost attachment snapshot";return false;}
        R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_0]);
        if(image_assets::Acquire(gfxRenderTargets[R_RENDERTARGET_RESOLVED_SCENE].image)!=firstTarget->color||render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_0])||render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_RESOLVED_SCENE])!=sharedTarget) {error_="Native target source release lost shared target";return false;}
        R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_RESOLVED_SCENE]);R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_1]);
        render_targets::Clear();
        const auto priorPrimary=gfxMetrics.shadowmapFormatPrimary,priorSecondary=gfxMetrics.shadowmapFormatSecondary;
        const auto priorBuild=gfxMetrics.shadowmapBuildTechType;
        gfxMetrics.shadowmapFormatPrimary=D3DFMT_R32F;gfxMetrics.shadowmapFormatSecondary=D3DFMT_D24X8;gfxMetrics.shadowmapBuildTechType=3;
        R_InitShadowmapRenderTarget(0,32,2,&gfxRenderTargets[R_RENDERTARGET_SHADOWMAP_SUN],SHADOWMAP_RT_TILE_VERTICAL);
        auto shadow=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_SHADOWMAP_SUN]);
        if(!shadow||!shadow->color||!shadow->depth||shadow->color->description.format!=VK_FORMAT_R32_SFLOAT
            ||shadow->color->description.extent.width!=32||shadow->color->description.extent.height!=64) {error_="Canonical native shadow attachments failed";return false;}
        AttachmentPass shadowPass(device);if(!shadowPass.Create(shadow->color->texture,shadow->depth->texture,true,error_))return false;
        R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_SHADOWMAP_SUN]);render_targets::Clear();
        gfxMetrics.shadowmapFormatPrimary=priorPrimary;gfxMetrics.shadowmapFormatSecondary=priorSecondary;gfxMetrics.shadowmapBuildTechType=priorBuild;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_shadow_target=1 r32f_primary=1 depth_secondary=1 width=32 height=64 actual_depth_format=%u real_framebuffer_created=1",unsigned(shadow->depth->description.format));
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_fullscreen_targets=1 rgba16f_and_rgba8=1 shared_depth=1 target_alias_survives_source_release=1");

        // Run the canonical full target list together, with fixture dimensions;
        // shadows retain their original default sizes. No game-init claim.
        const auto oldFloatZ=r_floatz,oldIntZ=r_allow_intz,oldNullRT=r_allow_null_rt;
        const auto priorReflection=r_reflectionProbeGenerate;const auto priorMinimal=g_allocateMinimalResources;
        const auto priorExtra=g_extraCamSettings;const auto priorSunSize=dx.sunShadowmapSize;
        const auto priorIntZ=dx.supportsIntZ;
        dvar_s floatZ{},allowIntZ{},nullRT{},reflectionTargets{};
        floatZ.current.enabled=allowIntZ.current.enabled=nullRT.current.enabled=true;
        r_floatz=&floatZ;r_allow_intz=&allowIntZ;r_allow_null_rt=&nullRT;r_reflectionProbeGenerate=&reflectionTargets;
        g_allocateMinimalResources=false;R_StoreNativeDeviceCaps(hardware);
        vidConfig.sceneWidth=vidConfig.displayWidth=64;vidConfig.sceneHeight=vidConfig.displayHeight=32;
        g_extraCamSettings.enabled=true;g_extraCamSettings.width=16;g_extraCamSettings.height=8;
        R_InitRenderTargets_PC();R_InitUI3DRenderTarget(16,8,0,true);
        auto initialSavedState=std::make_unique<GfxCmdBufState>(gfxCmdBufState);
        auto initialSavedSource=std::make_unique<GfxCmdBufSourceState>(gfxCmdBufSourceState);
        auto initialSavedInput=std::make_unique<GfxCmdBufInput>(gfxCmdBufInput);
        target_bindings::Clone(&gfxCmdBufState,initialSavedState.get());
        texture_bindings::Clone(&gfxCmdBufState,initialSavedState.get());constant_state::Clone(&gfxCmdBufState,initialSavedState.get());
        material_bindings::Clone(&gfxCmdBufState,initialSavedState.get());geometry_bindings::Clone(&gfxCmdBufState.prim,&initialSavedState->prim);
        dvar_s initialMin{},initialMax{},initialMip{},initialBias{},initialDisable{};initialMin.current.integer=initialMax.current.integer=1;
        const auto priorMin=r_texFilterAnisoMin,priorMax=r_texFilterAnisoMax,priorMip=r_texFilterMipMode,priorBias=r_texFilterMipBias,priorDisable=r_texFilterDisable;
        r_texFilterAnisoMin=&initialMin;r_texFilterAnisoMax=&initialMax;r_texFilterMipMode=&initialMip;r_texFilterMipBias=&initialBias;r_texFilterDisable=&initialDisable;
        if(pass_recording::Current()) {error_="Initial state fixture unexpectedly has a GPU pass scope";return false;}
        RB_SetInitialState();
        if(target_bindings::Acquire(&gfxCmdBufState)!=&gfxRenderTargets[R_RENDERTARGET_FRAME_BUFFER]
            ||gfxCmdBufState.renderTargetId!=R_RENDERTARGET_FRAME_BUFFER||gfxCmdBufState.prim.device||gfxCmdBufSourceState.renderTargetWidth!=64||gfxCmdBufSourceState.renderTargetHeight!=32) {
            error_="Canonical native initial state/desired framebuffer failed";return false;
        }
        GfxViewport initialViewport{1,1,32,16};R_SetViewport(&gfxCmdBufState,&initialViewport);
        if(gfxCmdBufState.viewport.x!=1||gfxCmdBufState.viewport.width!=32) {error_="Native pre-frame desired viewport failed";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_initial_state=pass real_framebuffer_selected=1 no_pass_scope_required=1 queued_viewport=1 no_COM_device=1");
        r_texFilterAnisoMin=priorMin;r_texFilterAnisoMax=priorMax;r_texFilterMipMode=priorMip;r_texFilterMipBias=priorBias;r_texFilterDisable=priorDisable;
        gfxCmdBufState=*initialSavedState;gfxCmdBufSourceState=*initialSavedSource;gfxCmdBufInput=*initialSavedInput;
        target_bindings::Clone(initialSavedState.get(),&gfxCmdBufState);target_bindings::Release(initialSavedState.get());
        texture_bindings::Clone(initialSavedState.get(),&gfxCmdBufState);texture_bindings::Release(initialSavedState.get());
        constant_state::Clone(initialSavedState.get(),&gfxCmdBufState);constant_state::Release(initialSavedState.get());
        material_bindings::Clone(initialSavedState.get(),&gfxCmdBufState);material_bindings::Release(initialSavedState.get());
        geometry_bindings::Clone(&initialSavedState->prim,&gfxCmdBufState.prim);geometry_bindings::Release(&initialSavedState->prim);

        unsigned targetCount=0;
        for(auto& target:gfxRenderTargets) {
            auto record=render_targets::Acquire(&target);if(!record)continue;
            if(target.surface.color||target.surface.depthStencil) {error_="Native target retained a D3D surface";return false;}
            AttachmentPass pass(device);
            if(!pass.Create(record->color?record->color->texture:nullptr,record->depth?record->depth->texture:nullptr,true,error_))return false;
            ++targetCount;
        }
        auto floatTarget=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_FLOAT_Z]);
        auto camera=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_MISSILE_CAM]);
        auto ui=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_UI3D]);
        if(dx.supportsIntZ||!floatTarget||!floatTarget->color||floatTarget->color->description.format!=VK_FORMAT_R32_SFLOAT
            ||!camera||!camera->depth||!ui||!ui->depth||camera->color->description.extent.width!=16) {
            error_="Canonical global native targets/fallback/sized targets failed";return false;
        }
        R_ShutdownRenderTargets();
        for(auto& target:gfxRenderTargets)if(target.image||render_targets::Acquire(&target)) {error_="Global target shutdown retained an owner";return false;}
        r_floatz=oldFloatZ;r_allow_intz=oldIntZ;r_allow_null_rt=oldNullRT;r_reflectionProbeGenerate=priorReflection;
        g_allocateMinimalResources=priorMinimal;g_extraCamSettings=priorExtra;dx.sunShadowmapSize=priorSunSize;dx.supportsIntZ=priorIntZ;
        vidConfig.sceneWidth=oldWidth;vidConfig.sceneHeight=oldHeight;vidConfig.displayWidth=oldDisplayWidth;vidConfig.displayHeight=oldDisplayHeight;
        r_loadForRenderer=oldTargetLoad;useFastFile=oldTargetFastFile;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_global_target_startup=1 target_count=%u all_real_framebuffers=1 floatz_shader_fallback=1 extra_camera_ui3d_depth=1 global_shutdown_owners_released=1 Com_Init_called=0",targetCount);

        if(!CheckMeshUploads(error_))return false;
        R_VertexStream2_Init(128);unsigned streamBytes{};
        auto* stream=R_VertexStream2_Lock(&streamBytes);
        if(!stream||streamBytes!=128) {error_="Native stream2 mapping size failed";return false;}
        std::memset(stream,0x31,128);R_VertexStream2_Unlock();
        auto firstStream=buffer_assets::Acquire(R_VertexStream2_GetVB());
        stream=R_VertexStream2_Lock(&streamBytes);stream[96]=0x72;R_VertexStream2_Unlock();
        auto nextStream=buffer_assets::Acquire(R_VertexStream2_GetVB());unsigned char before[128]{},after[128]{};
        if(firstStream->gpu==nextStream->gpu||!firstStream->gpu->Read(0,before,128,error_)||!nextStream->gpu->Read(0,after,128,error_))return false;
        for(unsigned i=0;i<128;++i)if(before[i]!=0x31||after[i]!=(i==96?0x72:0x31)) {error_="Native stream2 lost preserved bytes";return false;}
        const auto savedCount=s_world.draw.vertexCount,savedLayerBytes=s_world.draw.vertexLayerDataSize;
        unsigned int worldBytes[11]{},layerBytes[3]{};
        dvar_s worldLoad{};worldLoad.current.enabled=true;const auto previousLoad=r_loadForRenderer;r_loadForRenderer=&worldLoad;
        R_CreateWorldVertexBuffer(&s_world.draw.vd.worldVb,reinterpret_cast<int*>(worldBytes),sizeof(worldBytes));
        R_CreateWorldVertexBuffer(&s_world.draw.vld.layerVb,reinterpret_cast<int*>(layerBytes),sizeof(layerBytes));
        r_loadForRenderer=previousLoad;
        auto worldVB=buffer_assets::Acquire(&s_world.draw.vd.worldVb),layerVB=buffer_assets::Acquire(&s_world.draw.vld.layerVb);
        if(!worldVB||!layerVB) {error_="Canonical world buffer producer failed";return false;}
        s_world.draw.vertexCount=1;s_world.draw.vertexLayerDataSize=sizeof(layerBytes);
        R_ReleaseWorld();s_world.draw.vertexCount=savedCount;s_world.draw.vertexLayerDataSize=savedLayerBytes;
        if(buffer_assets::Acquire(&s_world.draw.vd.worldVb)||buffer_assets::Acquire(&s_world.draw.vld.layerVb)||buffer_assets::Acquire(R_VertexStream2_GetVB())) {error_="Native world unload retained registered buffer owners";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_stream2_preserved_update=1 world_vertex_layer_create_release=1 borrowed_after_world_release=1");

        R_InitStaticModelCache();
        struct CacheCleanup {~CacheCleanup(){R_ShutdownStaticModelCache();}} cacheCleanup;
        unsigned char patch[96];std::memset(patch,0x31,sizeof(patch));
        if(!buffer_assets::UpdateRange(&gfxBuf.smodelCacheVb,64,patch,sizeof(patch),error_))return false;
        auto firstCache=buffer_assets::Acquire(&gfxBuf.smodelCacheVb);
        std::memset(patch,0x72,32);
        if(!buffer_assets::UpdateRange(&gfxBuf.smodelCacheVb,96,patch,32,error_))return false;
        auto nextCache=buffer_assets::Acquire(&gfxBuf.smodelCacheVb);unsigned char prior[160]{},current[160]{};
        if(!firstCache||!nextCache||firstCache->gpu==nextCache->gpu
            ||!firstCache->gpu->Read(0,prior,sizeof(prior),error_)||!nextCache->gpu->Read(0,current,sizeof(current),error_))return false;
        for(unsigned i=0;i<160;++i)if(prior[i]!=(i<64?0:0x31)||current[i]!=(i<64?0:(i>=96&&i<128?0x72:0x31))) {error_="Native cache patch changed retained or unrelated bytes";return false;}
        std::string rejected;
        if(buffer_assets::UpdateRange(&gfxBuf.smodelCacheVb,firstCache->gpu->Size(),patch,32,rejected)) {error_="Native cache accepted out-of-range patch";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_static_cache_init=1 allocation_bytes=%llu range_patch_preserves_previous=1 unrelated_bytes_preserved=1 gap_zeroed=1",static_cast<unsigned long long>(firstCache->gpu->Size()));

        R_InitWaterSimulationVertexBuffers();
        R_FreeWaterSimulationVertexBuffers();
        R_CreateParticleCloudBuffer();
        auto cloudVB=buffer_assets::Acquire(&gfxBuf.particleCloudVertexBuffer),cloudIB=buffer_assets::Acquire(&gfxBuf.particleCloudIndexBuffer);
        if(!cloudVB||!cloudIB||cloudVB->payloadBytes!=65536||cloudIB->payloadBytes!=24576
            ||gfxBuf.particleCloudVertexBuffer||gfxBuf.particleCloudIndexBuffer) {error_="Canonical particle cloud buffers not published";return false;}
        R_DestroyParticleCloudBuffer();
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_water_init_free=1 particle_cloud_init_free=1 particle_vertex_bytes=%zu particle_index_bytes=%zu retained_after_free=1",cloudVB->payloadBytes,cloudIB->payloadBytes);

        R_StoreNativeDeviceCaps(hardware);
        dvar_s minAniso{},maxAniso{},mipMode{},bias{},disable{};
        minAniso.current.integer=maxAniso.current.integer=1;mipMode.current.integer=0;
        const auto oldMin=r_texFilterAnisoMin,oldMax=r_texFilterAnisoMax,oldMip=r_texFilterMipMode,oldSamplerBiasDvar=r_texFilterMipBias,oldDisable=r_texFilterDisable;
        r_texFilterAnisoMin=&minAniso;r_texFilterAnisoMax=&maxAniso;r_texFilterMipMode=&mipMode;r_texFilterMipBias=&bias;r_texFilterDisable=&disable;
        R_SetTexFilter();
        r_texFilterAnisoMin=oldMin;r_texFilterAnisoMax=oldMax;r_texFilterMipMode=oldMip;r_texFilterMipBias=oldSamplerBiasDvar;r_texFilterDisable=oldDisable;
        if(s_decodeSamplerFilterState[1]!=0x1101||s_decodeSamplerFilterState[2]!=0x2201) {error_="Canonical native sampler table mismatch";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_hardware_caps=1 device=%s api=%u.%u.%u vendor=%u max_texture=%u samplers=%u cube_mips=%u canonical_sampler_table=1 engine_init_claimed=0",
            hardware.properties.deviceName,VK_VERSION_MAJOR(hardware.properties.apiVersion),VK_VERSION_MINOR(hardware.properties.apiVersion),VK_VERSION_PATCH(hardware.properties.apiVersion),
            hardware.properties.vendorID,vidConfig.maxTextureSize,vidConfig.maxTextureMaps,unsigned(hardware.mipCubemaps));
        vertexShader_.name="native-preview-fullscreen";vertexShader_.prog.loadDef.program=const_cast<std::uint32_t*>(SharedVS);
        vertexShader_.prog.loadDef.programSize=sizeof(SharedVS)/4;
        pixelShader_.name="native-preview-colour";pixelShader_.prog.loadDef.program=const_cast<std::uint32_t*>(ColorPS);
        pixelShader_.prog.loadDef.programSize=sizeof(ColorPS)/4;
        if(!shader_assets::Create(&vertexShader_,error_)||!shader_assets::Create(&pixelShader_,error_))return false;
        vertexAsset_=shader_assets::Acquire(&vertexShader_);pixelAsset_=shader_assets::Acquire(&pixelShader_);
        if(!vertexAsset_||!pixelAsset_) {error_="Native shader ownership lookup failed";return false;}
        declaration_.streamCount=2;declaration_.routing.data[0]={0,0};declaration_.routing.data[1]={2,5};
        MaterialVertexDeclaration* declaration=&declaration_;
        Load_BuildVertexDecl(&declaration);
        declarationAsset_=vertex_assets::Acquire(&declaration_);
        if(!declarationAsset_||!declaration_.isLoaded) {error_="Canonical native declaration load failed";return false;}
        if(shader_assets::SelectNative(false)) {error_="Backend deselected with live native declaration";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_vertex_load=1 variants=18 borrowed_after_release=1");
        const std::string path=root_+"/native-vulkan-preview.bin";FILE* file=std::fopen(path.c_str(),"rb");
        if(!file) {error_="Native material capture fixture unavailable";return false;}
        unsigned header[3]{};const bool read=std::fread(header,4,3,file)==3;
        const bool compressed=header[0]==0x3342564e;
        if(!read||(header[0]!=0x3152564e&&!compressed)||!header[1]||!header[2]||header[1]>4096||header[2]>4096) {std::fclose(file);error_="Invalid capture header";return false;}
        std::vector<unsigned char> pixels(compressed?std::size_t((header[1]+3)/4)*((header[2]+3)/4)*16:std::size_t(header[1])*header[2]*4);
        const bool complete=std::fread(pixels.data(),1,pixels.size(),file)==pixels.size();const bool end=std::fgetc(file)==EOF;
        const bool closed=std::fclose(file)==0;if(!complete||!end||!closed) {error_="Capture size/read failure";return false;}
        // Capture bytes are RGBA; the canonical A8R8G8B8 upload contract is BGRA.
        if(!compressed)for(std::size_t i=0;i<pixels.size();i+=4)std::swap(pixels[i],pixels[i+2]);
        const auto sourceFormat=compressed?D3DFMT_DXT5:D3DFMT_A8R8G8B8;
        image_=std::make_unique<GfxImage>();image_->name="native-preview-image";
        Image_Create2DTexture_PC(image_.get(),static_cast<unsigned short>(header[1]),static_cast<unsigned short>(header[2]),1,0,sourceFormat);
        Image_UploadData(image_.get(),sourceFormat,D3DCUBEMAP_FACE_POSITIVE_X,0,pixels.data());
        imageAsset_=image_assets::Acquire(image_.get());
        if(!imageAsset_||imageAsset_->uploads.size()!=1||image_->texture.basemap) {error_="Canonical native image publication failed";return false;}
        if(shader_assets::SelectNative(false)) {error_="Backend deselected with live native image";return false;}
        commandState_=std::make_unique<GfxCmdBufState>();
        R_InitCmdBufState(commandState_.get());
        if(commandState_->activeStateBits[0]!=0x18004800||commandState_->activeStateBits[1]!=2||commandState_->prim.device) {
            error_="Canonical native command initialization mismatch";return false;
        }
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_command_init=1 default_state=18004800,2");
        materialPass_.vertexShader=&vertexShader_;materialPass_.pixelShader=&pixelShader_;materialPass_.vertexDecl=&declaration_;
        commandState_->pass=&materialPass_;
        R_SetPixelShader(commandState_.get(),&pixelShader_);R_UpdateVertexDecl(commandState_.get());
        const auto selected=material_bindings::Acquire(commandState_.get());
        if(!selected||selected->vertex!=vertexAsset_||selected->pixel!=pixelAsset_||selected->declaration!=declarationAsset_) {error_="Canonical material shader/declaration selection failed";return false;}
        shader_assets::Release(&vertexShader_);shader_assets::Release(&pixelShader_);vertex_assets::Release(&declaration_);
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_material_selection=1 shader_declaration_borrow_after_owner_release=1");
        // Preview supplies the same decoded linear filter table entry used by
        // canonical startup. Actual native global sampler startup is pending.
        // Canonical R_SetTexFilter supplies the actual decoded entry.
        const float testBias=-0.25f;const unsigned oldBias=dx.mipBias;std::memcpy(&dx.mipBias,&testBias,sizeof(testBias));
        const GfxCmdBufContext context{nullptr,commandState_.get()};
        R_SetSampler(context,0,0x62,image_.get());R_SetSamplerState(commandState_.get(),0,0x62);
        dx.mipBias=oldBias;
        textureBindings_=texture_bindings::Acquire(commandState_.get());
        if(!textureBindings_||textureBindings_->slots[0].image!=imageAsset_||textureBindings_->slots[0].bias!=testBias) {error_="Canonical native texture binding failed";return false;}
        if(gfxCmdBufState.prim.device||texture_bindings::Acquire(&gfxCmdBufState)) {error_="Preview command copy requires an isolated engine state";return false;}
        const GfxCmdBufState savedGlobal=gfxCmdBufState;gfxCmdBufState=*commandState_;
        texture_bindings::Clone(commandState_.get(),&gfxCmdBufState);material_bindings::Clone(commandState_.get(),&gfxCmdBufState);
        GfxCmdBufState local{};R_InitLocalCmdBufState(&local);
        const auto copied=texture_bindings::Acquire(&local);
        texture_bindings::Release(&local);material_bindings::Release(&local);texture_bindings::Release(&gfxCmdBufState);material_bindings::Release(&gfxCmdBufState);gfxCmdBufState=savedGlobal;
        if(copied!=textureBindings_) {error_="Canonical command copy lost native texture ownership";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_command_copy=1 copied_snapshot_after_release=1");
        // A descriptor submission retains the immutable snapshot even after
        // the command state's live slot is disabled and its owner released.
        GfxCmdBufState disabled{};texture_bindings::Clone(commandState_.get(),&disabled);
        R_DisableSampler(&disabled,0);texture_bindings::Release(&disabled);
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_sampler_set_update_disable=1 borrowed_bindings_after_release=1 mip_bias=-0.25");
        Image_Release(image_.get());
        if(image_assets::Acquire(image_.get())) {error_="Canonical native image release failed";return false;}
        texture_=imageAsset_->texture;const auto& uploaded=*imageAsset_->uploads[0].bytes;
        upload_=std::make_unique<Buffer>(physical,device);
        if(!upload_->Create(uploaded.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,error_)
            ||!upload_->Write(0,uploaded.data(),uploaded.size(),error_)
            ||!texture_->UploadSubresource(frame.command,upload_->Handle(),0,0,0,VK_IMAGE_LAYOUT_UNDEFINED,error_))return false;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_image_create_upload_release=1 borrowed_after_release=1 source_format=%u decoded_bytes=%zu revision=%llu",
            unsigned(sourceFormat),uploaded.size(),static_cast<unsigned long long>(imageAsset_->revision));
        VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(physical,&props);
        const auto alignment=props.limits.minUniformBufferOffsetAlignment;uniformAlignment_=alignment;
        pixelOffset_=((4096+alignment-1)/alignment)*alignment;vertexOffset_=65536;
        for(unsigned slot=0;slot<bo1::vulkan::Presentation::FrameSlots;++slot) {
            buffers_[slot]=std::make_unique<Buffer>(physical,device);
            if(!buffers_[slot]->Create(vertexOffset_+3*sizeof(GfxVertex),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,error_))return false;
            const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,8},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,4},{VK_DESCRIPTOR_TYPE_SAMPLER,4}};
            VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=8;pool.poolSizeCount=3;pool.pPoolSizes=sizes;
            if(!Check(vkCreateDescriptorPool(device,&pool,nullptr,&pools_[slot]),"material frame pool",error_))return false;
        }
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_material_texture=%ux%u bytes=%zu uploaded_once=1",header[1],header[2],pixels.size());
    }
    if(!programCache_) {
        programCache_=std::make_unique<ProgramCache>(device);
        if(!programCache_->Initialize(error_))return false;
    }
    if(generation_!=frame.generation)programCache_->Clear();
    if(!PrepareAttachments(frame))return false;
    if(generation_!=frame.generation) {
        // A generation change follows a complete presentation lifecycle drain.
        program_.reset();
        const auto selected=material_bindings::Acquire(commandState_.get());
        const std::vector<VkVertexInputBindingDescription> bindings={{0,sizeof(GfxVertex),VK_VERTEX_INPUT_RATE_VERTEX}};
        std::vector<VkVertexInputAttributeDescription> attributes;unsigned vertexUbyteMask=0;
        if(!vertex_assets::Resolve(*selected->declaration,selected->variant,selected->vertex->translation,physical,bindings,attributes,error_,&vertexUbyteMask))return false;
        ProgramDescription d;d.vertexUbyteMask=vertexUbyteMask;d.vertexBytecode=selected->vertex->bytecode.data();d.vertexBytes=selected->vertex->bytecode.size()*4;
        d.pixelBytecode=selected->pixel->bytecode.data();d.pixelBytes=selected->pixel->bytecode.size()*4;
        d.renderPass=presentation_.RenderPass();d.vertexBindings=bindings.data();d.vertexBindingCount=unsigned(bindings.size());
        d.vertexAttributes=attributes.data();d.vertexAttributeCount=unsigned(attributes.size());
        if(!ApplyMaterialState(commandState_->activeStateBits[0],commandState_->activeStateBits[1],d,error_))return false;
        program_=programCache_->Acquire(d,error_);if(!program_)return false;
        auto reused=programCache_->Acquire(d,error_);
        if(reused!=program_) {error_="Native pipeline cache failed identical-key reuse";return false;}
        generation_=frame.generation;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_program_cache_reuse=1 entries=%zu hits=%u misses=%u driver_cache=1",programCache_->Size(),programCache_->Hits(),programCache_->Misses());
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_material_pipeline_generation=%u surface_transform=%u",generation_,unsigned(presentation_.Transform()));
    }
    const float w=float(frame.extent.width),h=float(frame.extent.height);
    std::array<float,16> matrix={2/w,0,0,-1, 0,-2/h,0,1, 0,0,1,0, 0,0,0,1};
    presentation_.PreRotateProjection(matrix);
    GfxVertex vertices[3]{};
    const float xy[3][2]={{0,2*h},{0,0},{2*w,0}},uv[3][2]={{0,2},{0,0},{2,0}};
    for(unsigned i=0;i<3;++i) {vertices[i].xyzw[0]=xy[i][0];vertices[i].xyzw[1]=xy[i][1];vertices[i].xyzw[3]=1;
        vertices[i].texCoord[0]=uv[i][0];vertices[i].texCoord[1]=uv[i][1];}
    if(!vertexBufferAsset_) {
        geometry_=std::make_unique<XSurface>();
        Load_VertexBuffer(&geometry_->vb0,reinterpret_cast<unsigned char*>(vertices),sizeof(vertices));geometry_->flags=0x80;geometry_->triCount=1;
        static unsigned short indices[]={0,1,2};geometry_->triIndices=indices;
        XSurfaceOptimize(nullptr,geometry_.get());
        R_SetStaticModelVertexBuffer(&commandState_->prim,geometry_.get());
        R_SetStaticModelIndexBuffer(&commandState_->prim,geometry_.get());
        vertexBufferAsset_=buffer_assets::Acquire(&geometry_->vb0);indexBufferAsset_=buffer_assets::Acquire(&geometry_->indexBuffer);
        if(!vertexBufferAsset_||!indexBufferAsset_||geometry_->vb0||geometry_->indexBuffer
            ||vertexBufferAsset_->payloadBytes!=sizeof(vertices)||indexBufferAsset_->payloadBytes!=6||indexBufferAsset_->gpu->Size()!=32) {
            error_="Canonical native vertex/index buffer publication failed";return false;
        }
        std::string rejection;
        if(buffer_assets::Attach(VK_NULL_HANDLE,VK_NULL_HANDLE,rejection)) {error_="Buffer context detached while GPU owners survive";return false;}
        R_SetStreamsForXModelSurface(geometry_.get(),&commandState_->prim,nullptr);
        R_ChangeIndicesFromOwner(&commandState_->prim,&geometry_->indexBuffer);
        const auto bound=geometry_bindings::Acquire(&commandState_->prim);
        if(!bound||bound->streams[0].buffer!=vertexBufferAsset_||bound->indices!=indexBufferAsset_) {error_="Canonical native stream/index binding failed";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_model_stream_and_index_bind=1 stride=32");
        // Borrowed draw records retain actual VkBuffers after engine unload.
        XSurfaceUnoptimize(geometry_.get());buffer_assets::Release(&geometry_->vb0);
        if(buffer_assets::Acquire(&geometry_->vb0)||buffer_assets::Acquire(&geometry_->indexBuffer)) {error_="Native geometry owner not removed";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_buffer_load=1 vertex_bytes=%zu index_payload=%zu index_alloc=%llu borrowed_after_release=1",
            vertexBufferAsset_->payloadBytes,indexBufferAsset_->payloadBytes,static_cast<unsigned long long>(indexBufferAsset_->gpu->Size()));
    }
    if(!dynamicChecked_) {
        GfxVertexBufferState vb{};
        // Use the canonical one-element index pool so its reservation/wrap
        // pointer arithmetic remains within the actual owning array.
        const auto savedVB=gfxBuf.dynamicVertexBuffer;
        const auto savedIB=gfxBuf.dynamicIndexBuffer;
        const auto savedPool=gfxBuf.dynamicIndexBufferPool[0];
        gfxBuf.dynamicIndexBufferPool[0]={};
        struct Restore {
            GfxVertexBufferState* vb;GfxVertexBufferState* savedVB;GfxIndexBufferState* savedIB;GfxIndexBufferState savedPool;
            ~Restore() {
                buffer_assets::Release(&vb->buffer);buffer_assets::Release(&gfxBuf.dynamicIndexBufferPool[0].buffer);
                gfxBuf.dynamicVertexBuffer=savedVB;gfxBuf.dynamicIndexBuffer=savedIB;gfxBuf.dynamicIndexBufferPool[0]=savedPool;
            }
        } restore{&vb,savedVB,savedIB,savedPool};
        gfxBuf.dynamicVertexBuffer=&vb;gfxBuf.dynamicIndexBuffer=gfxBuf.dynamicIndexBufferPool;
        R_InitDynamicVertexBufferState(&vb,sizeof(vertices)*2);
        R_InitDynamicIndexBufferState(gfxBuf.dynamicIndexBuffer,6);
        unsigned short indices[3]={0,1,2};
        if(R_SetVertexData(commandState_.get(),vertices,3,sizeof(GfxVertex))!=0
            ||R_SetIndexData(&commandState_->prim,reinterpret_cast<unsigned char*>(indices),1)!=0) {error_="Native first dynamic upload offset failed";return false;}
        auto firstVB=buffer_assets::Acquire(&vb.buffer),firstIB=buffer_assets::Acquire(&gfxBuf.dynamicIndexBuffer->buffer);
        if(R_SetVertexData(commandState_.get(),vertices,3,sizeof(GfxVertex))!=sizeof(vertices)
            ||R_SetIndexData(&commandState_->prim,reinterpret_cast<unsigned char*>(indices),1)!=3) {error_="Native dynamic append offset failed";return false;}
        auto appendedVB=buffer_assets::Acquire(&vb.buffer),appendedIB=buffer_assets::Acquire(&gfxBuf.dynamicIndexBuffer->buffer);
        if(firstVB->gpu!=appendedVB->gpu||firstIB->gpu!=appendedIB->gpu
            ||firstVB->payloadBytes!=sizeof(vertices)||firstIB->payloadBytes!=6
            ||appendedVB->payloadBytes!=sizeof(vertices)*2||appendedIB->payloadBytes!=12) {error_="Native dynamic append publication failed";return false;}
        vb.used=0;
        if(R_SetVertexData(commandState_.get(),vertices,3,sizeof(GfxVertex))!=0
            ||R_SetIndexData(&commandState_->prim,reinterpret_cast<unsigned char*>(indices),1)!=0) {error_="Native dynamic discard offset failed";return false;}
        auto discardedVB=buffer_assets::Acquire(&vb.buffer),discardedIB=buffer_assets::Acquire(&gfxBuf.dynamicIndexBuffer->buffer);
        GfxVertex original[3]{};unsigned short originalIndices[3]{};
        if(firstVB->gpu==discardedVB->gpu||firstIB->gpu==discardedIB->gpu
            ||!firstVB->gpu->Read(0,original,sizeof(original),error_)||std::memcmp(original,vertices,sizeof(vertices))
            ||!firstIB->gpu->Read(0,originalIndices,sizeof(originalIndices),error_)||std::memcmp(originalIndices,indices,sizeof(indices))) {
            error_="Native dynamic discard overwrote retained backing";return false;
        }
        std::string rejection;
        if(buffer_assets::Write(&vb.buffer,1,vertices,sizeof(vertices),rejection)) {error_="Native dynamic buffer accepted overlap";return false;}
        if(R_SetVertexData(commandState_.get(),vertices,3,sizeof(GfxVertex))!=sizeof(vertices)) {error_="Native final dynamic append offset failed";return false;}
        commandState_->prim.streams[1].offset=4;commandState_->prim.streams[2].stride=8;
        R_SetStreamSourceFromOwner(&commandState_->prim,&vb.buffer,sizeof(vertices),sizeof(GfxVertex));
        auto bound=geometry_bindings::Acquire(&commandState_->prim);
        if(!bound||bound->streams[0].offset!=sizeof(vertices)||bound->streams[0].buffer!=buffer_assets::Acquire(&vb.buffer)
            ||commandState_->prim.streams[0].vb||commandState_->prim.streams[0].offset!=sizeof(vertices)
            ||commandState_->prim.streams[1].offset||commandState_->prim.streams[2].stride) {error_="Canonical owner-aware dynamic stream binding failed";return false;}
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_dynamic_stream=1 vertex_offset=%zu extra_streams_cleared=1",sizeof(vertices));
        dynamicChecked_=true;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_dynamic_buffers=1 append_same_backing=1 discard_retains_previous=1 vertex_bytes=%zu index_bytes=6 overlap_rejected=1",sizeof(vertices));
    }
    float constants[256][4]{};
    constants[5][0]=constants[8][0]=constants[11][0]=1;
    constants[6][1]=constants[7][2]=constants[9][1]=constants[10][2]=constants[20][1]=constants[21][2]=1;
    auto& buffer=*buffers_[frame.slot];
    for(unsigned row=0;row<4;++row)R_SetVertexShaderConstantFromLiteral(commandState_.get(),row,matrix.data()+row*4);
    if(!constantOwnershipChecked_)for(unsigned row=0;row<256;++row)R_SetPixelShaderConstantFromLiteral(commandState_.get(),row,constants[row]);
    if(!constantSource_)constantSource_=std::make_unique<GfxCmdBufSourceState>();
    std::memcpy(constantSource_->input.consts[0],matrix.data(),16);constantSource_->constVersions[0]=1;
    MaterialShaderArgument code{};code.dest=0;code.u.codeConst.index=0;code.u.codeConst.rowCount=1;
    const GfxCmdBufContext constantContext{constantSource_.get(),commandState_.get()};
    R_SetVertexShaderConstantFromCode_New(constantContext,&code);
    std::memcpy(constantSource_->input.consts[0],constants[5],16);constantSource_->constVersions[0]=2;code.dest=5;
    R_SetPixelShaderConstantFromCode(constantContext,&code);
    const auto snapshot=constant_state::Acquire(commandState_.get());
    if(!snapshot) {error_="Canonical native constant publication failed";return false;}
    if(!constantOwnershipChecked_) {
        GfxCmdBufState copied{};constant_state::Clone(commandState_.get(),&copied);
        if(constant_state::Acquire(&copied)!=snapshot) {error_="Native constant copy lost its immutable bank";return false;}
        const float changed[4]={1,2,3,4};R_SetPixelShaderConstantFromLiteral(commandState_.get(),255,changed);
        if(snapshot->floats[1][255][0]!=0||constant_state::Acquire(&copied)->floats[1][255][0]!=0) {error_="Captured constant snapshot changed after update";return false;}
        R_SetPixelShaderConstantFromLiteral(commandState_.get(),255,constants[255]);constant_state::Release(&copied);
        constantOwnershipChecked_=true;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_literal_and_code_constants=1 copied_snapshot_immutable=1");
    }
    constantFrames_[frame.slot]=snapshot;
    if(!buffer.Write(vertexOffset_,vertices,sizeof(vertices),error_))return false;
    if(frame.slot==0&&!probeChecked_)__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_literal_constants=1 immutable_banks_uploaded=1 revision=%llu",
        static_cast<unsigned long long>(snapshot->revision));
    if(!Check(vkResetDescriptorPool(device,pools_[frame.slot],0),"material frame retirement",error_))return false;
    auto draw=material_bindings::Capture(commandState_.get(),error_);if(!draw)return false;
    drawFrames_[frame.slot]=draw;
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(physical,&properties);
    VkDeviceSize cursor=0;std::vector<ProgramResource> vs,ps;
    if(!material_bindings::Upload(*draw,buffer,cursor,properties.limits.minUniformBufferOffsetAlignment,vs,ps,error_))return false;
    if(!drawCaptureChecked_) {
        const float changed[4]={1,2,3,4};R_SetPixelShaderConstantFromLiteral(commandState_.get(),255,changed);
        auto later=material_bindings::Capture(commandState_.get(),error_);if(!later)return false;
        std::vector<ProgramResource> laterVS,laterPS;
        if(!material_bindings::Upload(*later,buffer,cursor,properties.limits.minUniformBufferOffsetAlignment,laterVS,laterPS,error_))return false;
        float captured[4]{};
        if(!buffer.Read(ps[0].buffer.offset+255*16,captured,sizeof(captured),error_)||captured[0]!=0
            ||draw->constants->floats[1][255][0]!=0||later->constants->floats[1][255][0]!=1||laterPS[0].buffer.offset==ps[0].buffer.offset) {
            error_="Later material capture overwrote a prior uniform slice";return false;
        }
        R_SetPixelShaderConstantFromLiteral(commandState_.get(),255,constants[255]);drawCaptureChecked_=true;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_material_draw_capture=1 independent_uniform_slices=1 prior_ps_offset=%llu later_ps_offset=%llu",
            static_cast<unsigned long long>(ps[0].buffer.offset),static_cast<unsigned long long>(laterPS[0].buffer.offset));
    }
    dispatchCursors_[frame.slot]=cursor;
    return program_->BindResources(pools_[frame.slot],vs,ps,sets_[frame.slot],error_);
}
bool NativeMaterialPreview::PrepareAttachments(const bo1::vulkan::FrameContext& frame) {
    using namespace bo1::vulkan;const auto device=presentation_.Device();const auto physical=presentation_.Physical();
    if(probeReadback_) {
        if(!probeChecked_&&frame.slot==probeSlot_) {
            // Presentation has retired this CPU slot's submission fence.
            if(R_HW_IsFencePending(&engineFence_)) {error_="Native submitted event remained pending after marker retirement";return false;}
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_event_fence=pass startup_pool9=1 no_COM=1 recorded_pending=1 submitted_marker_completed=1");
            unsigned engineSamples=777;
            if(RB_HW_ReadOcclusionQueryFromOwner(&engineQuery_,&engineSamples,0)!=S_OK||engineSamples!=16) {error_="Native engine owner query read failed";return false;}
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_query_owner=pass allocated_without_COM=1 actual_samples=%u retired_frame_read=1",engineSamples);
            std::uint64_t counts[5]{},generation{};
            for(unsigned i=0;i<5;++i)if(occlusion_->Read(i,counts[i],generation,error_)!=QueryRead::Ready) {if(error_.empty())error_="Native retired query result unavailable";return false;}
            if(counts[0]||counts[1]||!counts[2]||!counts[3]||counts[4]||(occlusion_->Precise()&&(counts[2]!=16||counts[3]!=16))) {error_="Native occlusion GPU visibility/sample count mismatch";return false;}
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_occlusion_queries=pass precise=%u counts=%llu,%llu,%llu,%llu,%llu generation=%llu submission_fence_retired=1 unsubmitted_result_rejected=1",unsigned(occlusion_->Precise()),(unsigned long long)counts[0],(unsigned long long)counts[1],(unsigned long long)counts[2],(unsigned long long)counts[3],(unsigned long long)counts[4],(unsigned long long)generation);
            unsigned char bytes[40]{};
            if(!probeReadback_->Read(0,bytes,sizeof(bytes),error_))return false;
            const unsigned depth=unsigned(bytes[4])|(unsigned(bytes[5])<<8);
            if(bytes[0]<63||bytes[0]>65||bytes[1]<127||bytes[1]>129||bytes[2]<190||bytes[2]>193||bytes[3]!=255||depth<32766||depth>32769||bytes[8]<95||bytes[8]>97||bytes[9]<111||bytes[9]>113||bytes[10]<174||bytes[10]>176||bytes[11]<63||bytes[11]>65||bytes[12]||bytes[13]||bytes[16]!=1||bytes[20]!=1||bytes[24]!=0||bytes[28]||bytes[29]) {
                error_="Native alpha/depth/blend or attachment LOAD readback mismatch";return false;
            }
            if(bytes[32]<63||bytes[32]>65||bytes[33]<127||bytes[33]>129||bytes[34]<190||bytes[34]>193||bytes[35]!=255) {
                error_="Native generated attachment sample GPU readback mismatch";return false;
            }
            const unsigned sharedDepth=unsigned(bytes[36])|(unsigned(bytes[37])<<8)|(unsigned(bytes[38])<<16);
            if(sharedDepth<0x7ffffe||sharedDepth>0x800001) {error_="Native shared depth lost contents during color target switch";return false;}
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_target_switch_clear_viewport=pass color_load_preserved=1 independent_depth_load_preserved=1 depth24=%u",sharedDepth);
            VkImageLayout submittedLayout{};
            if(!image_usage::Sample(generatedSource_,submittedLayout,error_))return false;
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_generated_attachment_sample=pass gpu_rgba=%u,%u,%u,%u no_cpu_upload=1 committed_layout=%u active_feedback_rejected=1",bytes[32],bytes[33],bytes[34],bytes[35],unsigned(submittedLayout));
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_depth_only_gpu_draw=pass no_color_attachments=1 depth_written=%u",unsigned(bytes[28])|(unsigned(bytes[29])<<8));
            probeChecked_=true;
            __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_state_pipeline=1 alpha_discard_depth_reject=pass load_blend_depthwrite=pass stencil_front_and_cull=pass before=%u,%u,%u,%u,%u after=%u,%u,%u,%u,%u",
                bytes[0],bytes[1],bytes[2],bytes[3],depth,bytes[8],bytes[9],bytes[10],bytes[11],unsigned(bytes[12])|(unsigned(bytes[13])<<8));
        }
        return true;
    }
    GfxImage color{},depth{};color.name="native-attachment-color";depth.name="native-attachment-depth";
    Image_Create2DTexture_PC(&color,4,4,1,0x20000,D3DFMT_A8B8G8R8);
    Image_Create2DTexture_PC(&depth,4,4,1,0x20000,D3DFMT_D16);
    GfxRenderTarget target{};
    R_AssignImageToRenderTargetColor(&target,&color);R_AssignImageToRenderTargetDepthStencil(&target,&depth);
    probeTarget_=render_targets::Acquire(&target);
    render_targets::Release(&target);Image_Release(&color);Image_Release(&depth);
    if(!probeTarget_||!probeTarget_->color||!probeTarget_->depth) {error_="Canonical native attachment publication failed";return false;}
    probePass_=std::make_unique<AttachmentPass>(device);
    if(!probePass_->Create(probeTarget_->color->texture,probeTarget_->depth->texture,true,error_))return false;
    scratchDepth_=std::make_shared<Texture2D>(physical,device);
    TextureDescription transient;transient.extent={4,4,1};transient.format=VK_FORMAT_D16_UNORM;
    transient.aspects=VK_IMAGE_ASPECT_DEPTH_BIT;transient.linearSampling=false;
    transient.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    if(!scratchDepth_->Create(transient,error_))return false;
    scratchPass_=std::make_unique<AttachmentPass>(device);
    if(!scratchPass_->Create(probeTarget_->color->texture,scratchDepth_,false,error_))return false;
    probeReadback_=std::make_unique<Buffer>(physical,device);
    if(!probeReadback_->Create(40,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,error_))return false;
    // Actual T5 VS with a tiny SM3 fixture PS returning (.75,.25,.5,.25).
    static const unsigned constantPS[]={0xffff0300,0x05000051,0xa00f0000,0x3f400000,0x3e800000,0x3f000000,0x3e800000,
        0x02000001,0x800f0800,0xa0e40000,0x0000ffff};
    const VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,14};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=14;pool.poolSizeCount=1;pool.pPoolSizes=&poolSize;
    if(!Check(vkCreateDescriptorPool(device,&pool,nullptr,&statePool_),"state probe descriptor pool",error_))return false;
    loadPass_=std::make_unique<AttachmentPass>(device);
    if(!loadPass_->Create(probeTarget_->color->texture,probeTarget_->depth->texture,true,error_,true))return false;
    const std::vector<VkVertexInputBindingDescription> bindings={{0,sizeof(GfxVertex),VK_VERTEX_INPUT_RATE_VERTEX}};
    std::vector<VkVertexInputAttributeDescription> attributes;unsigned vertexUbyteMask=0;
    if(!vertex_assets::Resolve(*declarationAsset_,0,vertexAsset_->translation,physical,bindings,attributes,error_,&vertexUbyteMask))return false;
    GfxImage stencil{};stencil.name="native-stencil-state-probe";
    Image_Create2DTexture_PC(&stencil,4,4,1,0x20000,D3DFMT_D24S8);
    stencilDepth_=image_assets::Acquire(&stencil);Image_Release(&stencil);
    if(!stencilDepth_) {error_="Canonical D24S8 owner unavailable";return false;}
    stencilPass_=std::make_unique<AttachmentPass>(device);
    if(!stencilPass_->Create(probeTarget_->color->texture,stencilDepth_->texture,true,error_))return false;
    depthOnlyPass_=std::make_unique<AttachmentPass>(device);
    if(!depthOnlyPass_->Create(nullptr,probeTarget_->depth->texture,true,error_))return false;
    for(unsigned mode=0;mode<7;++mode) {
        unsigned words[2]={0x18004800,2};
        if(mode==0){words[0]=0x18007000;words[1]=5;} // alpha >=128 rejects .25; LESS + depth write.
        if(mode==1)words[1]=9; // alpha off, EQUAL rejects z0 against .5.
        if(mode==2){words[0]|=0x165|(0x112u<<16);words[1]=5;} // src alpha RGB; alpha ONE/ZERO.
        if(mode>=3&&mode<6) {
            words[1]=0xe00e06c2u; // front INC_WRAP, back KEEP, ALWAYS compare, z disabled.
            if(mode==4)words[0]=0x18008800; // D3DCULL_CCW: retain clockwise primitive.
            if(mode==5)words[0]=0x1800c800; // D3DCULL_CW: cull clockwise primitive.
        }
        if(mode==6)words[1]=5; // Depth LESS + write, no color attachment.
        R_SetState(commandState_.get(),words);
        ProgramDescription description;description.vertexUbyteMask=vertexUbyteMask;description.vertexBytecode=vertexAsset_->bytecode.data();description.vertexBytes=vertexAsset_->bytecode.size()*4;
        description.pixelBytecode=constantPS;description.pixelBytes=sizeof(constantPS);description.colorAttachment=mode!=6;description.renderPass=mode==6?depthOnlyPass_->RenderPass():(mode<3?probePass_->RenderPass():stencilPass_->RenderPass());
        description.vertexBindings=bindings.data();description.vertexBindingCount=unsigned(bindings.size());description.vertexAttributes=attributes.data();description.vertexAttributeCount=unsigned(attributes.size());
        if(!ApplyMaterialState(commandState_->activeStateBits[0],commandState_->activeStateBits[1],description,error_))return false;
        statePrograms_[mode]=programCache_->Acquire(description,error_);
        if(!statePrograms_[mode])return false;
        const std::vector<ProgramResource> vs={{ResourceKind::Constants,0,{buffers_[frame.slot]->Handle(),0,4096},{}}};
        const std::vector<ProgramResource> ps={{ResourceKind::Constants,0,{buffers_[frame.slot]->Handle(),pixelOffset_,4096},{}}};
        if(!statePrograms_[mode]->BindResources(statePool_,vs,ps,stateSets_[mode],error_))return false;
    }
    unsigned restored[2]={0x18004800,2};R_SetState(commandState_.get(),restored);
    occlusion_=std::make_unique<OcclusionQueries>(device,presentation_.PreciseOcclusionEnabled());
    if(!occlusion_->Create(5,error_)||!occlusion_->Reset(frame.command,error_))return false;
    const auto draw=[&](unsigned mode) {
        const int query=mode<4?int(mode):mode==5?4:-1;
        if(query>=0&&!occlusion_->Begin(frame.command,unsigned(query),error_))return false;
        if(mode==6&&RB_HW_IssueOcclusionQueryFromOwner(&engineQuery_,D3DISSUE_BEGIN)!=S_OK) {error_="Native engine query begin failed";return false;}
        statePrograms_[mode]->Bind(frame.command,stateSets_[mode]);
        VkViewport viewport{0,4,4,-4,0,1};VkRect2D scissor{{0,0},{4,4}};
        vkCmdSetViewport(frame.command,0,1,&viewport);vkCmdSetScissor(frame.command,0,1,&scissor);
        const VkBuffer vertex=buffers_[frame.slot]->Handle();vkCmdBindVertexBuffers(frame.command,0,1,&vertex,&vertexOffset_);vkCmdDraw(frame.command,3,1,0,0);
        if(mode==6&&RB_HW_IssueOcclusionQueryFromOwner(&engineQuery_,D3DISSUE_END)!=S_OK) {error_="Native engine query end failed";return false;}
        return query<0||occlusion_->End(frame.command,unsigned(query),error_);
    };
    VkClearColorValue clear{{0.25f,0.5f,0.75f,1.0f}};
    probePass_->Begin(frame.command,clear,0.5f);if(!draw(0)||!draw(1)){probePass_->End(frame.command);return false;}probePass_->End(frame.command);
    const auto copy=[&](const std::shared_ptr<Texture2D>& image,VkImageAspectFlags aspect,VkImageLayout old,VkDeviceSize offset) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=image->Handle();
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={aspect,0,1,0,1};
        barrier.oldLayout=old;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask=aspect==VK_IMAGE_ASPECT_COLOR_BIT?VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(frame.command,aspect==VK_IMAGE_ASPECT_COLOR_BIT?VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        VkBufferImageCopy region{};region.bufferOffset=offset;region.imageSubresource={aspect,0,0,1};region.imageExtent={1,1,1};
        vkCmdCopyImageToBuffer(frame.command,image->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,probeReadback_->Handle(),1,&region);
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=old;barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(frame.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    };
    copy(probeTarget_->color->texture,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,0);
    copy(probeTarget_->depth->texture,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,4);
    loadPass_->Begin(frame.command,clear,0.5f);if(!draw(2)){loadPass_->End(frame.command);return false;}loadPass_->End(frame.command);
    copy(probeTarget_->color->texture,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,8);
    copy(probeTarget_->depth->texture,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,12);
    scratchPass_->Begin(frame.command,clear,0.25f);scratchPass_->End(frame.command);
    for(unsigned mode=3;mode<6;++mode) {
        stencilPass_->Begin(frame.command,clear,0.5f,0);if(!draw(mode)){stencilPass_->End(frame.command);return false;}stencilPass_->End(frame.command);
        VkImageMemoryBarrier stencilBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};stencilBarrier.image=stencilDepth_->texture->Handle();
        stencilBarrier.srcQueueFamilyIndex=stencilBarrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        stencilBarrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
        stencilBarrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;stencilBarrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        stencilBarrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;stencilBarrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(frame.command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&stencilBarrier);
        VkBufferImageCopy region{};region.bufferOffset=16+(mode-3)*4;region.imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};region.imageExtent={1,1,1};
        vkCmdCopyImageToBuffer(frame.command,stencilDepth_->texture->Handle(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,probeReadback_->Handle(),1,&region);
        stencilBarrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;stencilBarrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        stencilBarrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;stencilBarrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(frame.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&stencilBarrier);
    }
    depthOnlyPass_->Begin(frame.command,clear,0.5f);if(!draw(6)){depthOnlyPass_->End(frame.command);return false;}depthOnlyPass_->End(frame.command);
    copy(probeTarget_->depth->texture,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,28);
    GfxImage generatedOutput{};generatedOutput.name="native-generated-output";
    dvar_s sourceLoad{},sourceFastFile{};sourceLoad.current.enabled=sourceFastFile.current.enabled=true;const auto priorSourceLoad=r_loadForRenderer,priorSourceFastFile=useFastFile;r_loadForRenderer=&sourceLoad;useFastFile=&sourceFastFile;
    R_InitSizedRenderTargetImage(7,4,4,0,D3DFMT_A8B8G8R8,RENDERTARGET_USAGE_RENDER,&gfxRenderTargets[R_RENDERTARGET_PINGPONG_0]);
    R_InitSizedRenderTargetImage(8,4,4,0,D3DFMT_A8B8G8R8,RENDERTARGET_USAGE_RENDER,&gfxRenderTargets[R_RENDERTARGET_PINGPONG_1]);
    r_loadForRenderer=priorSourceLoad;useFastFile=priorSourceFastFile;
    auto canonicalSource=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_0]);
    auto canonicalOther=render_targets::Acquire(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_1]);
    if(!canonicalSource||!canonicalOther||canonicalSource->depth!=canonicalOther->depth) {error_="Canonical fixture depth sharing failed";return false;}
    generatedSource_=canonicalSource->color->texture;generatedSharedDepth_=canonicalSource->depth->texture;
    Image_Create2DTexture_PC(&generatedOutput,4,4,1,0x20000,D3DFMT_A8B8G8R8);
    generatedOutput_=image_assets::Acquire(&generatedOutput)->texture;
    GfxCmdBufState generatedState{};
    if(!texture_bindings::Set(&generatedState,0,gfxRenderTargets[R_RENDERTARGET_PINGPONG_0].image,R_DecodeSamplerState(97),0,error_))return false;
    generatedBindings_=texture_bindings::Acquire(&generatedState);texture_bindings::Release(&generatedState);Image_Release(&generatedOutput);
    GfxCmdBufState targetState{};GfxCmdBufSourceState targetSource{};targetSource.renderTargetWidth=targetSource.renderTargetHeight=4;
    GfxCmdBufContext targetContext{&targetSource,&targetState};GfxViewport targetViewport{0,0,4,4};
    {
        VkDeviceSize targetCursor=0;
        pass_recording::RecordingScope recording(frame.command,physical,device,*programCache_,*buffers_[frame.slot],pools_[frame.slot],targetCursor,uniformAlignment_,dispatchFrames_[frame.slot],passFrames_[frame.slot]);
        R_SetRenderTarget(targetContext,R_RENDERTARGET_PINGPONG_0);R_SetViewport(&targetState,&targetViewport);
        R_ClearScreen(nullptr,3,clear.float32,0.5f,0,nullptr);
        VkImageLayout layout{};std::string rejected;
        if(image_usage::Sample(generatedSource_,layout,rejected)) {error_="Active generated attachment sampling was accepted";return false;}
        R_SetRenderTarget(targetContext,R_RENDERTARGET_PINGPONG_1);R_SetViewport(&targetState,&targetViewport);
        const float otherClear[4]={0,0,0,0};R_ClearScreen(nullptr,1,otherClear,1,0,nullptr);
        R_SetRenderTarget(targetContext,R_RENDERTARGET_PINGPONG_0);R_SetViewport(&targetState,&targetViewport);
        if(!recording.Finish(error_))return false;
    }
    R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_0]);
    R_ShutDownSingleRenderTarget(&gfxRenderTargets[R_RENDERTARGET_PINGPONG_1]);render_targets::Clear();
    generatedOutputPass_=std::make_unique<AttachmentPass>(device);
    if(!generatedOutputPass_->Create(generatedOutput_,nullptr,false,error_))return false;
    static const unsigned samplePS[]={0xffff0300,0x0200001f,0x80000005,0x90030000,
        0x0200001f,0x90000000,0xa00f0800,0x03000042,0x800f0000,0x90e40000,0xa0e40800,
        0x02000001,0x800f0800,0x80e40000,0x0000ffff};
    ShaderTranslation generatedTranslation;
    if(!TranslateShader(samplePS,sizeof(samplePS),generatedTranslation)) {error_=generatedTranslation.diagnostics;return false;}
    ProgramDescription generatedDescription;generatedDescription.vertexBytecode=vertexAsset_->bytecode.data();generatedDescription.vertexBytes=vertexAsset_->bytecode.size()*4;
    generatedDescription.pixelBytecode=samplePS;generatedDescription.pixelBytes=sizeof(samplePS);generatedDescription.renderPass=generatedOutputPass_->RenderPass();
    generatedDescription.vertexBindings=bindings.data();generatedDescription.vertexBindingCount=unsigned(bindings.size());
    generatedDescription.vertexAttributes=attributes.data();generatedDescription.vertexAttributeCount=unsigned(attributes.size());
    generatedProgram_=programCache_->Acquire(generatedDescription,error_);if(!generatedProgram_)return false;
    const VkDescriptorPoolSize generatedSizes[]={{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1},{VK_DESCRIPTOR_TYPE_SAMPLER,1}};
    VkDescriptorPoolCreateInfo generatedPool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};generatedPool.maxSets=2;generatedPool.poolSizeCount=3;generatedPool.pPoolSizes=generatedSizes;
    if(!Check(vkCreateDescriptorPool(device,&generatedPool,nullptr,&generatedPool_),"generated sample pool",error_))return false;
    std::vector<ProgramResource> generatedVS={{ResourceKind::Constants,0,{buffers_[frame.slot]->Handle(),0,4096},{}}},generatedPS;
    if(!texture_bindings::AppendResources(*generatedBindings_,generatedTranslation,generatedPS,error_))return false;
    std::array<VkDescriptorSet,2> generatedSets;
    if(!generatedProgram_->BindResources(generatedPool_,generatedVS,generatedPS,generatedSets,error_))return false;
    VkClearColorValue generatedEmpty{{0,0,0,0}};
    if(!generatedOutputPass_->BeginTracked(frame.command,generatedEmpty,error_))return false;
    generatedProgram_->Bind(frame.command,generatedSets);
    VkViewport generatedViewport{0,4,4,-4,0,1};VkRect2D generatedScissor{{0,0},{4,4}};
    vkCmdSetViewport(frame.command,0,1,&generatedViewport);vkCmdSetScissor(frame.command,0,1,&generatedScissor);
    const VkBuffer generatedVertex=buffers_[frame.slot]->Handle();vkCmdBindVertexBuffers(frame.command,0,1,&generatedVertex,&vertexOffset_);vkCmdDraw(frame.command,3,1,0,0);
    if(!generatedOutputPass_->EndTracked(frame.command,error_))return false;
    copy(generatedOutput_,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,32);
    copy(generatedSharedDepth_,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,36);
    VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(frame.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
    std::uint64_t unpublished=123,generation=999;std::string pendingError;
    if(occlusion_->Read(2,unpublished,generation,pendingError)!=QueryRead::Pending||unpublished!=123||generation!=999) {error_="Unsubmitted native query published a fabricated result";return false;}
    unsigned pendingOwner=456;
    if(RB_HW_ReadOcclusionQueryFromOwner(&engineQuery_,&pendingOwner,0)!=S_FALSE||pendingOwner!=456) {error_="Native engine query exposed an unsubmitted count";return false;}
    R_HW_InsertFence(&engineFence_);
    if(engineFence_||!R_HW_IsFencePending(&engineFence_)) {error_="Native recorded event returned a fabricated completion";return false;}
    queriesPending_=true;probeSlot_=frame.slot;return true;
}
void NativeMaterialPreview::Draw(void* self,const bo1::vulkan::FrameContext& frame) {
    auto& owner=*static_cast<NativeMaterialPreview*>(self);if(!owner.error_.empty())return;

    VkViewport viewport{0,float(frame.extent.height),float(frame.extent.width),-float(frame.extent.height),0,1};VkRect2D scissor{{0,0},frame.extent};
    vkCmdSetViewport(frame.command,0,1,&viewport);vkCmdSetScissor(frame.command,0,1,&scissor);
    // Explicit diagnostic controls; production draw requires canonical dvars.
    dvar_s floor{},cap{},skip{};const auto oldFloor=r_drawPrimFloor,oldCap=r_drawPrimCap,oldSkip=r_skipDrawTris;
    r_drawPrimFloor=&floor;r_drawPrimCap=&cap;r_skipDrawTris=&skip;
    bo1::vulkan::material_dispatch::Pass target;target.handle=owner.presentation_.RenderPass();
    bo1::vulkan::material_dispatch::RecordingScope recording(owner.commandState_.get(),frame.command,owner.presentation_.Physical(),target,
        *owner.programCache_,*owner.buffers_[frame.slot],owner.pools_[frame.slot],owner.dispatchCursors_[frame.slot],owner.uniformAlignment_,owner.dispatchFrames_[frame.slot]);
    const GfxDrawPrimArgs args{3,1,0};R_DrawIndexedPrimitive(&owner.commandState_->prim,&args);
    r_drawPrimFloor=oldFloor;r_drawPrimCap=oldCap;r_skipDrawTris=oldSkip;
    if(owner.dispatchFrames_[frame.slot].empty())owner.error_="Canonical material dispatcher did not retain its submitted packet";
    else if(!owner.dispatchChecked_) {
        const auto& packet=owner.dispatchFrames_[frame.slot].back();
        if(!packet.geometry||!packet.textures||!packet.program||!packet.sets[0]||!packet.sets[1])owner.error_="Native dispatch packet incomplete";
        else {owner.dispatchChecked_=true;__android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","canonical_native_material_dispatch=1 automatic_pipeline_descriptor_uniform_geometry=1 complete_packet_retained_until_fence=1 cache_hits=%u",owner.programCache_->Hits());}
    }

}
}
