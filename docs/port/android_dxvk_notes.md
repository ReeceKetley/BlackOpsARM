# Android rendering path: DXVK + SDL3 (notes, 2026-10-10)

Decision direction (not final): keep BO1's existing D3D9 renderer, run it on DXVK (D3D9 -> Vulkan) with SDL3 as the platform/WSI layer,
instead of converting meshes/materials to Godot. Target GPU: **Mali** (user's device), so this repo is directly relevant.

## Reference repo

https://github.com/molotovgit/CnC-Generals-ZeroHour-Android
Build guide: `docs/BUILDING-DXVK-ANDROID.md` (read via summarizer only; not yet read in full).

What the guide says:
- DXVK built with **meson**, cross-compiled with the Android NDK clang (**NDK 27.2.12479018, API 28**, aarch64) via a meson cross file.
- Cross-file defines: `VK_ENABLE_BETA_EXTENSIONS`, `VK_USE_PLATFORM_ANDROID_KHR`; host system declared as `linux` (meson has no Android value DXVK accepts).
- WSI backend is **SDL3**: needs a real `pkgconf` and a cross-compiled `sdl3.pc`.
- Outputs `libdxvk_d3d8.so` / `libdxvk_d3d9.so` (versioned names like `.so.0.20600`); strip the version suffix for Gradle `jniLibs` (Android packages only `lib*.so`).
- Mobile-GPU patches (runtime capability checks, not `#ifdef ANDROID`): software **BC/DXT decoding**, dummy descriptors when `nullDescriptor` is missing,
  **serialized pipeline creation**, clip-plane avoidance, tolerate persistent `VK_SUBOPTIMAL_KHR`. Details are in the repo's `DISCOVERIES.md`.
- Lineage: doitsujin/dxvk -> fbraz3 DXVK/GeneralsX fork -> this repo.
- **Only the Mali-G57 is confirmed working.** Adreno, PowerVR, Xclipse unverified.

Not known yet (read the repo): how the game loads the .so (direct link vs dlopen of a d3d9 shim), D3DX handling, performance, full limitation list, DXVK version/commit.

## Things BO1 does that Generals probably does not (risk list for the spike)
- SM3.0 vertex/pixel shaders from the fastfile (MaterialVertexShader/PixelShader bytecode), many techsets, vertex declarations.
- Heavy render-target / post-process use, depth textures, MSAA, shadow maps.
- `d3dx9_43` use (textures, matrices, maybe shader helpers): needs a replacement (e.g. Wine's d3dx9 code) or porting the few calls.
- Engine renderer code is still Win32-layout (crashes on x64 right after "Getting Direct3D 9 interface"); needs 64-bit cleanup regardless of the API underneath.

## Plan
1. Fix the x64 renderer on Windows against real D3D9 (64-bit cleanup).
2. Same build through DXVK on Windows (d3d9.dll drop-in).
3. Build the repo's DXVK recipe for arm64; render something BO1 on the Mali device.
4. SDL3 behind the platform interfaces for window/input/audio/lifecycle.
Fallback if DXVK-on-Mali fails: native GLES/Vulkan renderer fed by the same fastfile DX9 data (still no Godot-style asset conversion).
