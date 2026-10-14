# Native GPU pass profile plan

2026-10-14; bounded source audit only. Root owns live SDK471/PID8567 measurement, device, builds and Ledger/STATE. No engine/provider/reference/assets changed. Rendering/playable goal remains unfinished.

## Same-process canonical isolation

Query each current value first. Change one flag to 0, close console/IME, sample the unchanged camera after transient work settles, restore the queried value and verify restoration before the next test. These remove CPU submission/preparation as well as GPU work; changes in FPS/GPU span are causal pass-group deltas, not pure shader-cost measurements. No restart or cheat/protection bypass is needed.

| Flag (default; registration flags) | Actual gate | Limits |
|---|---|---|
| sm_enable (1; 0) | r_scene.cpp:4092 R_GetAllowShadowMaps -> R_DynamicShadowType -> viewInfo dynamicShadowType (3624,3679); rb_draw3d.cpp:755-760 gates sun and spot shadow-map draws | Also removes frontend shadow/caster preparation and changes lit shading. Empty shadow lists can yield no delta. |
| r_enableDlights (1; 0x4000) | rb_draw3d.cpp:915-920 gates R_DrawLights | Multipass dynamic lights only, not main lit/lightgrid/environment lighting. A room without relevant lists can yield no delta. |
| r_dof_enable (1; 0x4001) | r_scene.cpp:4180-4217: if r_dof_tweak is 0, disabled value zeros viewInfo DOF; rb_postfx.cpp:8 predicate becomes false, 2439 skips RB_ApplyDepthOfField | First read r_dof_tweak: if enabled, this flag is overridden; do not modify the cheat tweak to force a result. Other post effects and bloom remain. Scene DOF may already be inactive. |
| r_depthPrepass (1; 1) | rb_draw3d.cpp:815-824 gates main non-missile depth prepass | Separate Float Z prepass at777-790 remains when !supportsIntZ or missile camera. Lit pass remains and may get more overdraw. Existing source comment says depth prepass is mandatory for some rendering (trees on mp_mountain); use only a brief room diagnostic, restore, do not adopt globally. |

Registration: r_dvars.cpp:1399,2587,1783,1364. Dvar_CanChangeValue (dvar.cpp:1753-1785) protects bits0x40(read-only),0x10(write protected),0x80(cheat). External latch bit0x20 is checked at1617. These four have none of those bits; bit0x4000 is not a settings-protection bypass.

Exclude r_drawWorld/r_drawSModels/r_drawSkinnedModels etc: flags0x80 (r_dvars.cpp:1036-1043). Do not enable cheats. r_distortion is writable but unsuitable to isolate resolves here: r_rendercmds.cpp:2114-2117 only sets rg.distortion; bounded gfx search found no other reader. R_ResolveDistortion at rb_draw3d.cpp:1216-1239 has no flag check and its calls (866,921) remain. r_drawSun is ordinary (flags1) but only suppresses sun sprite/flare/blind/glare, not sun lighting or shadow maps; lower-priority fallback.

## Pass boundaries already in source

RB_StandardDrawCommands rb_draw3d.cpp:718-974 already calls RB_BO1GpuMark(0..6). Useful groups are: pre-view work -> mark0; UI3D/shadows -> mark1; clears/Float Z/main depth/cloak prepass -> mark2; main lit/IntZ resolve/distortion resolve/reflected/post-resolve lit -> mark3; alpha/decal/sun/corona/IntZ/dlights/distortion resolve -> mark4; emissive -> mark5; waypoints/brightening/cloak/end-scene/late post effects -> mark6. These are groups, not individual shader stages; only first marks per frame are retained (rb_backend.cpp:5219-5228), so later/missile views fall into the tail rather than getting separate marks.

Legacy ring at rb_backend.cpp:4748-4891 is gated by g_bo1ProfOn/bo1_frameperf, independent of current Android overlay. Do not simply switch it on as an assumed safe provider: RB_BO1GpuFrameBegin clears/reuses a set still pending after8frames (4872-4875), unlike current overlay's no-pending-reuse ring. Root can reuse the existing mark positions with the current bounded epoch/lifetime provider if phase measurement becomes needed.

## What current GPU span measures

Android gpu::Begin is before backend scene/2D commands (rb_backend.cpp:4622-4630); BeforePresent is immediately before swapchain Present (5502). performance_gpu_overlay.cpp has8 sets, begins disjoint+timestamp, closes timestamp/frequency/disjoint, validates frequency/end>=begin, and GetData flags0. It measures elapsed GPU timestamp span across submitted commands, including idle/queue gaps inside the begin/end interval and work/barrier/resolve waits. It is not active GPU utilization, shader-only work, or wall-time Present duration. Time before the begin timestamp executes and after the end timestamp executes is outside this span. Independent KGSL busy-window evidence is required for busy utilization.

Protected DXVK-Native was inspected read-only. D3D9Query::Issue -> D3D9DeviceEx::Begin/End (d3d9_device.cpp:6273-6296) emits ordered CS work; GetData flags0 does not enter D3DGETDATA_FLUSH path (d3d9_query.cpp:150-176). End only invokes FlushImplicit for event/stalling queries. Fresh timestamps polled without FLUSH do not inherently force submission.

DxvkContext::writeTimestamp (dxvk_context.cpp:2677-2679) directly invokes query manager, with no spillRenderPass. DxvkGpuQueryManager::writeTimestamp (dxvk_gpu_query.cpp:331-351) resets a query then writes VK_PIPELINE_STAGE_ALL_COMMANDS_BIT. cmdResetQuery (dxvk_cmdlist.h:642-662) resets through host EXT call or InitBuffer; cmdWriteTimestamp (760-766) records on ExecBuffer. Thus additional pass timestamps do not inherently split render passes in this provider. They still add query/reset/command overhead and all-commands timestamp ordering can perturb pipeline overlap. Label phase results elapsed spans, validate total and overhead against untouched baseline, retain flags0 polling and never reuse pending query sets.