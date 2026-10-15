# Native Vulkan shader foundation — stage 1

2026-10-14. Goal remains complete Android D3D9 replacement, optimized for mobile, portable backend design and tablet visual acceptance. This stage implements the first native shader module, not the complete renderer or a measured speedup. Pre-native checkpoint: `4247b269`, branch `checkpoint/pre-native-vulkan`.

## Implemented

`src/gfx_vulkan/shader_translate.{h,cpp}` translates untouched T5 SM3 bytecode to Vulkan 1.1 SPIR-V through the already vendored vkd3d-shader 1.17. The module has no D3D9 or Android API dependency; the initial build harness is Android ARM64. This avoids retaining DXVK's D3D9 runtime for shader translation. Compiler licensing remains in the existing vendored tree.

Reflection owns its strings/descriptors/signatures after compiler storage is freed. Descriptor sets are separated by stage (VS0/PS1); bindings preserve float/int/bool constant-bank identity and texture/sampler indices. Input bytecode is copied before typed access so unaligned ARM asset addresses are safe. Unsupported descriptor layouts/stages fail explicitly; unsuccessful translation clears previously published code.

`TranslateShaderPair` links VS outputs to fragment inputs by semantic and component coverage. The actual fullscreen VS writes TEXCOORD0 in output register1 while both actual pixel shaders read input register0. The linked SPIR-V correctly decorates the VS texcoord as Location0; unlinked modules alone were insufficient for rendering. Cache/pipeline keys must include this linked interface when later implemented.

The existing compiler rejected `lrp` in the real colour shader. The narrow change to vendored `spirv.c` maps it to GLSL.std.450 FMix with reversed outer operands: D3D `a*b+(1-a)*c` becomes `mix(c,b,a)`. Existing swizzle/modifier/mask/destination handling is retained. No compiler assertions or engine sanitizers were removed. The modified compiler was built and transferred to a separate harness directory; the installed game provider remains unchanged.

## Tablet evidence

Focused Debug/UBSan-no-recovery translator and GPU harness on actual Adreno610 Vulkan1.1.128:

| Check | Result |
|---|---|
| Actual fullscreen VS | Translated and vkCreateShaderModule succeeds;1,672bytes |
| Actual bloom PS | Translated and module created;4,236bytes |
| Actual colour PS | Translated and module created;5,320bytes |
| Actual colour VS/PS semantic pair | Linked VS texcoord Location0 confirmed by disassembly |
| All four exported modules | `spirv-val --target-env vulkan1.1` passes |
| Unaligned actual colour bytecode | Same SPIR-V as aligned input |
| Invalid input length | Explicit rejection; prior output cleared |
| Native graphics LRP fixture | Real Vulkan pipeline/draw/fence/readback;four RGBA32F pixels match0,0.6875,-0.125,0 within1e-6 |

The LRP fixture includes factors -1 and1.5, so reversed operands or clamping would fail. It uses one submission with an owner fence, an explicit attachment-to-transfer dependency and transfer-to-host barrier. Memory type requirements are checked; upload/readback is coherent mapped memory. No per-frame queue/device idle; device wait appears only in failed-submit cleanup. This is a small math/resource-ownership check, not a benchmark or rendering of the whole game. Exact fixture values are appropriate for diagnosing this operation; game visual acceptance need not be pixel-identical.

Evidence: `build-android-audit/native_vulkan_shader2/tablet-final.log`, `manifest.json`, four exported `.spv/.spvasm` and validation files. Reproduction: `python tools/port/run_native_vulkan_shader_smoke.py --build build-android-audit/native_vulkan_shader2 --remote /data/local/tmp/boarm-vulkan-shader2`. This verifies upload hashes, transfers changed harness binaries only and checks the installed game PID stays unchanged (30744). No game assets transferred or APK restarted. Build via `android/cmake/native-vulkan` with NDK30/ARM64/API28/C++static/Debug; Ninja path must be explicit on this host.

The first prebuilt-compiler harness failed colour translation with four E2007 unhandled-LRP instructions; its original compiler/harness and build directory remain preserved. Original configure also failed because Ninja was not on PATH; the corrected configuration specifies CMAKE_MAKE_PROGRAM. These are resolved foundation failures, not full-backend completion.

## Next stage and remaining scope

Next use the linked actual T5 shader pair, real descriptor/constant layout and texture data in a native material draw. Then connect SDL Vulkan surface/swapchain presentation and the optional engine backend seam. Stage-specific constants require correct buffer size/alignment and integer/bool representation; alpha-test, fog, clip-plane and other material parameters require explicit compiler/pipeline mapping. Whole-library shader coverage is not established by these three representative programs.

Still pending: real game material visual draw, native presentation/resize/surface loss, image/buffer/resource providers, full material state/pipeline cache, frame/pass lifetimes, genuine frontend and gameplay, native overlay/performance profiling and measured mobile improvements. Existing D3D9 game remains the runtime reference. No30FPS/40–60FPS claim; active goal incomplete.
