# Original fog, material constants and cubemap/screenshot integration

2026-10-13. Full engine unlinked; actual Com_Init/Com_Frame incomplete.

Full target selects canonical r_fog.cpp, r_material_consts.cpp and r_screenshot.cpp.
Original fog state/interpolation, pixel-constant comparisons and cubemap/shots/
lighting bodies now compile. Original Windows algorithms and accepted reference
feature behavior remain unchanged. No renderer/device-ready flags forced.

Native screenshot adaptation captures the game surface rather than a Win32 desktop
monitor. platform/android/back_buffer_capture.h uses actual swap-chain GetBackBuffer,
surface description and format checks, rectangle/output-arithmetic validation,
optional actual multisample CreateRenderTarget/StretchRect resolve, system-memory
surface creation/GetRenderTargetData/LockRect readback, BGR/BGRA row/pitch copying,
UnlockRect and automatic COM releases. Failed operations return false. Caller
must own width*height*bytesPerPixel output bytes; the legacy signature supplies no
capacity. Unlock failure can leave copied output but still reports failure.
No synthetic pixels/window geometry or successful GPU fallback.

Native R_GetFrontBufferData retains the reference headless-client rejection before
actual game capture. Native R_TakeScreenshot uses the existing original game
back-buffer screenshot function, which performs real D3D readback/image-save calls.
The Android path does not invoke Win32 MonitorFromWindow/ClientToScreen. Existing
real scoped Sleep serves cubemap read/filter waits. Windows desktop path unchanged.

The native D3DX API header adds only the actual image format enum and image-save
declaration, checked against local June2010 SDK d3dx9tex.h. It does not implement
D3DXSaveSurfaceToFileA; that genuine image-encoding/file-I/O dependency remains
unresolved. CubeMapGen ErrorMsg retains Windows wtypes include on Windows; Android
uses types already supplied by DXVK's Windows declarations. No dialog or image
save function substituted.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link360.log records wtypes include error;
  link361 records D3DX include; link362 records Win32 monitor/Sleep dependencies;
  link363 compiles final original/native capture sources and fails real remaining
  dependencies. No runtime failure was hidden by those compile repairs.
- fog-material-cubemap-definitions1.log confirms original fog/material/cubemap/
  capture owners. cubemap-real-dependencies1.log verifies actual unresolved D3DX
  save and CubeMapProcessor constructor/destructor/init/filter/readback services.
- takeover_fog_material_cubemap_windows1: three normal whole TUs x86/x64 exit0,
  final source manifests retained.

No live D3D device/swap chain, GPU resolve/readback, lock failure, image encoder/file
save or CubeMapProcessor filtering executed. Native capture source compilation is
not GPU behavior proof. Initialized fog/material constants/shots/lighting and
resource/window/device recovery remain unverified; dimensions, packed pixel
conversions, decompiled consumer buffers and platform ownership need runtime audit.
No mirrored null-only fixture, unchanged device tests or asset transfers repeated.
Image save/CubeMapGen, audio/cinematic/renderer lifecycle/input/debug/Steam/Monkey
and other true dependencies remain. Full Com_Init has not returned.
