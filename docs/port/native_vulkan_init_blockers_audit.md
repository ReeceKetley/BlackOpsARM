# Native Vulkan: blocking audit of real engine startup

2026-10-14. Read-only audit of the working tree on android-arm64. No source, CMake,
ledger or STATE edits were made. Every claim below is from reading the cited code or
ledger entry; nothing was built, run or measured by this audit. Line numbers are from
the working tree at audit time and will drift.

## Verdict

The native resource, target, pipeline and per-draw layers are largely in place and exercised
by diagnostic fixtures. **The real engine cannot start on the native backend yet** because
backend selection, device/window creation, the D3D query objects, and the whole frame layer
(scene begin/end, render-target binding, viewport, clear, resolve, present) are still D3D9 and
nothing in production selects native. Every `shader_assets::Active()` branch is currently
dead outside the fixture. All native device runs remain `Com_Init_called=0`.

## STATE.md is behind the ledger

STATE.md's top entry is stage 24 and says full `R_InitRenderTargets_PC` "still reaches unported
framebuffer/INTZ/nullcolor/shadow branches". The tree and ledger are later:

- `r_rendertarget.cpp` now guards the framebuffer (`R_InitFrameBufferRenderTarget`), INTZ
  (`R_InitIntZRenderTarget_PC`, selects the R32F fallback), null-color
  (`R_InitSceneNullRenderTarget_PC`, depth-only share), shadowmap secondary attachments, sized
  UI3D/extra-camera targets and `R_ShutdownRenderTargets`.
- FACT-0447 (stage 27): canonical `R_InitRenderTargets_PC` plus sized targets create 26 native
  targets with real framebuffers, and global shutdown releases every owner.
- FACT-0448 (stage 28): `PipelineKey` + `ProgramCache` with a real `VkPipelineCache`.
- FACT-0449 (stage 29): canonical `R_DrawIndexedPrimitive` reaches the tablet GPU through
  material capture, cache, uniforms and descriptors.

So target startup is no longer a blocker. STATE.md should be updated in place at handoff.

## Blockers, in the order they will be hit

### 1. Nothing selects the native backend in production
`shader_assets::SelectNative(true)` is called only from
`platform/android/native_vulkan_material_preview.cpp:109`. `SelectNative` also refuses to switch
while any vertex/image/buffer owner exists (`shader_assets.cpp:41`), so selection must happen
before `R_InitGraphicsApi` creates anything. Needs an explicit policy (D3D9 stays the default per
GOAL.md) read before `R_PreCreateWindow`.

### 2. Device, window and swapchain are D3D9/DXVK
- `R_InitGraphicsApi` -> `R_PreCreateWindow` -> `R_CreateGameWindow` -> `R_InitHardware` ->
  `R_CreateDevice` -> `R_CreateDeviceInternal` (`r_init.cpp:1290,1334,1859,1899,383,538`) create a
  D3D9 device via `dx.d3d9->CreateDevice`. The native `Presentation` owner
  (`native_vulkan_entry.cpp`, `gfx_vulkan/presentation.*`) is a separate harness that is not
  reachable from `R_Init`.
- `R_StoreNativeDeviceCaps` (`r_init.cpp:2712`) exists but is called only from the preview
  fixture (lines 176 and 250). In production nothing fills `vidConfig`/`gfxMetrics` from the
  Vulkan device, including the shadowmap format fields `R_InitShadowmapRenderTarget` depends on.
- D3D9 capability probes run unconditionally before any native branch: `R_GetDepthStencilFormat`
  (`r_init.cpp:413`, `r_rendertarget.cpp:68-90`) and `R_SetupAntiAliasing`
  (`r_init.cpp:524`) call `dx.d3d9->CheckDevice*`.
- `R_FinishAttachingToWindow` asserts `dx.windows[n].swapChain` (`r_init.cpp:1976`) and
  `RB_SwapBuffers` asserts `dx.targetWindowIndex < dx.windowCount` (`rb_backend.cpp:5517`). The
  native path has no `IDirect3DSwapChain9`, so window bookkeeping needs a native owner, not a
  fake swapchain object.

### 3. Event and occlusion queries in `R_CreateForInitOrReset`
`r_init.cpp:2014-2032` creates 9 D3D event queries (`flushGpuQuery`, 8 `fencePool`) and, if not
minimal, sun/corona/super-flare/pixel-count occlusion queries via `RB_HW_AllocOcclusionQuery`
(`r_init.cpp:2043`). All are unguarded `dx.device->CreateQuery`, so they fault on a null device.
`R_Init` also runs `RB_CalcSunSpriteSamples` (`r_init.cpp:1282`) which depends on them.
Decision needed, not a shortcut: implement Vulkan query pools with the same semantics, or take
an explicit, documented unsupported path. The existing "low-quality sun visibility" fallback
(`r_init.cpp:1283`) is engine behavior, but it must not be used to hide a missing query
implementation. The fence pool consumers were not audited.

### 4. The frame layer is unported
Unguarded D3D calls in the per-frame path (counts from a grep of the call names; guarded call
sites are not subtracted, so treat as upper bounds except where a function is named):

| Area | Where |
|---|---|
| Scene begin/end | `rb_backend.cpp:4671,6020`, `rb_imagetouch.cpp:32,71`, `rb_sky.cpp:61,119` |
| Present / lost device | `rb_backend.cpp:5537` (`swapChain->Present`), `5427,5641,5728` (`TestCooperativeLevel`) |
| Render-target bind | `R_HW_SetRenderTarget` (`r_state.cpp:3003`, `SetRenderTarget` at 3022) |
| Viewport, clear | `R_SetViewport` (`r_state.cpp:2533`), `R_ClearScreen` (`r_state.cpp:3246`) |
| Resolves | `R_Resolve` (`rb_backend.cpp:789`), `R_ResolveIntZ_PC` (747), StretchRect (1472) |
| Post effects, 3D draw | `rb_postfx.cpp` (6), `rb_draw3d.cpp` (16 incl. the 0x80-byte vertex-buffer probe at 635-708) |
| Misc | `r_screenshot.cpp`, `r_texturemem.cpp:51` and `rb_backend.cpp:5216` (`GetAvailableTextureMem`), `r_model_lighting.cpp:1070` (`GetDeviceCaps`), `rb_backend.cpp:1867` (`SetGammaRamp`) |

`r_state.cpp` already has native branches for `R_DrawIndexedPrimitive`, `R_ChangeState_0/1`,
sampler set/disable, mesh stream and owner-based index/stream binds, and `r_shade.cpp` has ~10.
The state-change side is therefore mostly done; render-target, viewport, clear, resolve and
scene boundaries are not.

### 5. No engine code opens a native frame or render pass
`material_dispatch::RecordingScope` (`material_dispatch.h:18`) is an RAII object built by the
caller with a `VkCommandBuffer`, render-pass description, descriptor pool and uniform buffer. Its
only constructors are in fixtures. `R_DrawIndexedPrimitive` uses it when `HasScope()` is true
(`r_state.cpp:1044`); in the real frame loop nothing creates one. Required:
- A frame owner that acquires the swapchain image, records, submits, retires on fence and presents
  (reusing `Presentation`'s semaphores/fences).
- Mapping `R_SetRenderTarget`/clear/resolve to render-pass begin/end with correct load/store and
  layout transitions. This is where the doc's "scene depth is shared, shadow output is sampled"
  constraints (native_vulkan_backend_audit_plan.md) bite.
- GPU-generated image readiness/layout tracking (already listed as pending in STATE.md).

### 6. Known unsupported features that real content will hit
From STATE.md, unchanged by this audit: wireframe, polygon bias / nonzero stateOverrides,
dynamic stencil refs/masks, integer/bool constant banks, separate VS texture bank,
multistream/instancing, optional-feature and format fallbacks. `Packed` vertex formats: the
decoder in `gfx_vulkan/packed_vertex_decode.*` is CPU-only and has no GPU path or -512 oracle
proof, per FACT-0448, so the vertex-format mapping to `VkFormat` is still open.

## Suggested order

1. Decide the backend-selection policy and put it ahead of `R_PreCreateWindow` (blocker 1).
2. Native device/window owner behind `R_CreateDevice`, feeding `R_StoreNativeDeviceCaps` and
   replacing the D3D9 capability probes (blocker 2).
3. Query decision and implementation (blocker 3), since `R_CreateForInitOrReset` cannot return
   without it.
4. Frame owner + `R_HW_SetRenderTarget`/viewport/clear/begin/end/present (blockers 4 and 5), at
   which point the first real `Com_Init` -> `R_Init` -> one frame becomes testable.
5. Resolve/post-effect passes, the unsupported-feature list, then measured mobile optimization.

## Not verified by this audit
- Whether `R_InitImages`, `Material_Init`, `R_InitFonts`, `R_InitStaticModelCache` and
  `R_InitModelLightingImage` reach any other D3D call beyond `r_model_lighting.cpp:1070`.
- The consumers of `dx.fencePool` / `dx.flushGpuQuery`.
- Whether the Windows build is unaffected by the native guards (not built here).
- Anything about runtime behavior: all conclusions are static.

## Addendum: init callees and fence/query consumers

Follow-up read-only trace of the items listed under "Not verified". Static reading only.

### Fences and queries (refines blocker 3)
The 8-entry `dx.fencePool` is not only a wait primitive; it paces buffer reuse.
- `R_HW_InsertFence` (`cl_compositing.cpp:218`) hands out `fencePool[nextFence++ % 8]` and issues
  it. Callers: `RB_EndSceneRendering` -> `backEndData->endFence` every frame (`rb_draw3d.cpp:285`),
  `RB_SwapBuffers` -> `dx.swapFence[frame % gpuCount]` (`rb_backend.cpp:5722`), and
  `s_compositingFence` (`cl_compositing.cpp:190`).
- `R_HW_IsFencePending` (`cl_compositing.cpp:363`) polls with `GetData(..., D3DGETDATA_FLUSH)`.
  Real readers: `R_EndFencePending` (`r_scene.cpp:2127`), which gates the front end reusing
  `backEndData`, and the compositing job state (`cl_compositing.cpp:309`).
- The pool wraps at 8 with no check that a slot is still referenced, so a slot can be re-issued
  while an earlier holder still watches it. D3D tolerates that. A native replacement must define
  the behavior explicitly: tie each handle to a frame-slot `VkFence` retired by the frame owner,
  and never hand out a slot that is still pending without an ownership rule.
- `dx.swapFence[]` is written (`rb_backend.cpp:5722`) and cleared (`r_init.cpp:338-341`) but has
  no reader in the sources searched; it only consumes a pool slot.
- `dx.flushGpuQuery` is created (`r_init.cpp:2015`), read only by `R_FinishGpuFence`
  (`r_setstate_d3d.cpp:9`) and released. It is never `Issue`d anywhere in the sources searched,
  so on D3D9 `GetData` on an unissued event returns `S_OK` and the wait is a no-op. Its callers:
  `rb_backend.cpp:6035`, only when `dx.gpuSync` is set, which `R_UpdateGpuSyncType` forces to 0
  (`r_init.cpp:380`); and `R_SyncGpu` (`r_init.cpp:2470`), which has no callers (definition and
  comments only). The Android `R_FinishGpuFence` already aborts when the query is missing.
  Native recommendation: do not create the query. Make `R_FinishGpuFence` fail explicitly if it is
  ever reached natively, or implement a real device wait if a caller appears. Do not return
  success without waiting.
- Occlusion queries (sun/corona/super-flare/pixel-count) were not changed by this addendum.

### New blocker found: `RB_SetInitialState` (`rb_state.cpp:17`)
- Line 21 sets `gfxCmdBufState.prim.device = dx.device`, but the native branch of
  `R_InitCmdBufState` raises "Invalid native command-state initialization" when
  `state->prim.device` is non-null (`r_state_utils.cpp:69-70`). Natively `dx.device` must stay
  null, or the assignment must be skipped.
- Line 23 `R_SetInitialContextState(dx.device)` (`r_state.cpp:287`) calls
  `device->SetRenderState(...)` unguarded, so it dereferences null natively.
- Lines 28 and 31 `iassert` that the frame buffer's `surface.color` and `surface.depthStencil`
  are non-null. Native targets deliberately keep both null (the fixture treats a retained D3D
  surface as an error, `native_vulkan_material_preview.cpp:181`). These need native-aware checks
  against `render_targets::Acquire`, not removal.
- Line 41 `R_SetRenderTarget` ends in `R_HW_SetRenderTarget` (D3D `SetRenderTarget`,
  `r_state.cpp:3022`), already listed under blocker 4.

### Other init callees
| Callee | Finding |
|---|---|
| `R_InitModelLightingImage` | `dx.device->GetDeviceCaps` for `MaxVolumeExtent` (`r_model_lighting.cpp:1070`), unguarded. Needs the Vulkan `maxImageDimension3D`; not checked whether `HardwareInfo` exposes it. |
| `R_InitFonts` | Assertion only. No device use. |
| `R_InitDebug` | CPU limits only. |
| `R_StreamInit` / `R_StreamAlloc_InitTempImages` | CPU only (`DB_AllocTempImage` + memset, `r_streamalloc.cpp:8`). |
| `RB_StreamInit`, `RB_InitSceneViewport` | CPU only. |
| `R_InitScene` | Draw-surface pointer table setup; only the first ~30 lines were read. |
| `R_InitImages` | `R_SetPicmip`, `R_InitCodeImages` (`Image_Register` of `$white`, `$black`, ...), `RB_InitImages`, `R_InitRawImage`. The register calls go through `Image_*` paths that have native guards in `r_image.cpp`; not traced that every generated image takes one. |
| `RB_InitCodeImages` | Pointer table copying `gfxRenderTargets[..].image`. Correct only because `R_CreateForInitOrReset` (targets) runs before `R_InitSystems` -> `R_InitImages` in `R_InitHardware`. `R_InitGraphicsApi`'s second call (`dx.device` already set) calls only `R_InitSystems`. |
| `RB_BindDefaultImages` | Uses `R_SetSampler`, which has a native branch. |
| `Material_Init` | Registers 122 built-in materials plus one; each goes through the FastFile material and shader path. `r_material.cpp:527` (`CreateVertexDeclaration`) is a D3D call; not traced whether native skips it (guards exist near `r_material.cpp:24` and `:681`). |
| `RB_RegisterBackendAssets` | `R_RegisterFont("fonts/smalldevfont", 1)`, i.e. a material load. |

### Still not verified
`R_InitLoadWater`, `R_InitLightDefs`, `R_ClearFogs`, `R_InitWind` (names suggest CPU only, not read);
the rest of `R_InitScene`; the queued shader creators in `rb_resource.cpp:368-392` (STATE.md says
no callers, not re-checked); and everything about runtime behavior.
