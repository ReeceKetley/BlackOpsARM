# Android rendering path: DXVK + SDL3 (updated2026-10-11)

## Current focus and Windows evidence

User currently targets Samsung S25 EU and later Honor X8a Mali, but explicitly focuses on Windows Vulkan first (DEC-0013). Unrestricted Windows x64 already plays round-based Zombies through native D3D9 and now through official DXVK3.1.1/Vulkan. Pentagon full PaP and grenade routes pass. Current validation work isolates implicit overlays and fixes an upstream DXVK pipeline cleanup omission; see vulkan_windows.md for the latest tested package and remaining caveats. No native Android build or phone compatibility is established. Earlier notes below are reference research, not a current build recipe.


Decision direction (not final): keep BO1's existing D3D9 renderer, run it on DXVK (D3D9 -> Vulkan) with SDL3 as the platform/WSI layer,
instead of converting meshes/materials to Godot. Target GPU: **Mali** (user's device), so this repo is directly relevant.

## Reference repo

https://github.com/molotovgit/CnC-Generals-ZeroHour-Android
Build guide `docs/BUILDING-DXVK-ANDROID.md` was read in full from an ignored sparse clone at commitdec9083f6bbe5c032867f7b0cac0c7e07932d708. DISCOVERIES.md was partially reviewed; the whole repository has not been audited.

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
- That earlier x64 renderer-init crash is fixed; current D3D9 and Vulkan rendered tests reach gameplay and PaP. Remaining native warning/silent-stride and cold/developer-path audit still matters independently of the API.

## Plan
1. Keep Win32 and unrestricted native x64 regressions as the engine baseline.
2. Finish Windows Vulkan validation, repeatable launcher/package and native DXVK lifecycle fixes.
3. Later select an Android DXVK branch/features from actual S25/Honor capability data; this older fork recipe is a reference, not automatically the chosen version.
4. Port native platform/WSI/D3DX/audio/input/lifecycle boundaries and build ARM64. Windows DLL success alone does not supply these pieces.
Fallback if DXVK-on-Mali fails: native GLES/Vulkan renderer fed by the same fastfile DX9 data (still no Godot-style asset conversion).
