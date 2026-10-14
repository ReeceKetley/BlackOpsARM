# Pinned native CPU HLSL compiler bringup

2026-10-13. Actual Com_Init remains incomplete. The standalone CPU compiler now
builds and runs on HONOR; the engine's D3DXCompileShader wrapper and CTAB ownership
are still missing. Latest full entry link remains121; compiler target is separate.

Prepared unmodified Wine vkd3d1.17 source archive from
https://dl.winehq.org/vkd3d/source/vkd3d-1.17.tar.xz, SHA256
bc61cb9e84d5045cbcaffbdd707940d399d8bf62874663dfe5809a0bfb87e9b6.
Pinned Khronos SPIRV-Headers commit aa6cef192b8e693916eb713e7a9ccadf06062ceb
(vulkan-sdk-1.4.313.0 tag) supplies enum headers, not a Vulkan runtime or driver.
Preparation script verifies archive/header hashes and preserves license/source
files; third_party/native-shader-compiler/source-manifest.json records provenance.
These vendored sources are untracked pending current no-commit authorization.

Standalone android/cmake/native-shader-compiler builds the complete upstream
shader source list and the common helpers it uses, without the separate Vulkan
device implementation. Common blob.c is not required by this shader C API; it
belongs to the wider D3D12 object API. Existing MSYS host Bison3.8.2/Flex2.6.4
generate original grammars in the build directory; upstream source is unchanged.
Android feature macros are established with real NDK compile/header/symbol probes.
Optional external SPIRV-Tools is not enabled. The library is dynamically linked
and upstream COPYING/AUTHORS/notices are preserved for subsequent packaging.

ARM64/API28 Release shared library successfully links. ELF dynamic dependencies
are libm.so/libdl.so/libc.so only (shader-compiler-dependencies1.log), with no
Vulkan library dependency. DXVK Native1.9.2 and tablet driver requirements are
unchanged. The SPIR-V emitter's enum headers do not imply its output is used by
the existing game renderer; the consumed output here is D3D9 SM3 bytecode.

HONOR shader-compiler-run1 executes actual upstream compiler API:
matrix-transform vertex HLSL produces1232 bytes vs_3_0, tint/interpolant pixel HLSL
produces268 bytes ps_3_0. Both have exact SM3 stage headers, END tokens, bounded
instruction lengths and one embedded CTAB comment. Broken HLSL returns a real
error diagnostic and no success bytecode. API-owned messages/code are freed.
This is a normal Release smoke, not UBSan, full shader compatibility, Windows
bytecode parity, GPU execution or engine initialization. Only library and smoke
binary were pushed; staged retail assets were reused.

Reproduce after running python tools/port/prepare_native_shader_compiler.py:
configure android/cmake/native-shader-compiler with SDK CMake3.22.1/Ninja,
NDK30.0.16248370 android.toolchain.cmake, arm64-v8a/android-28/Release into
build-android-audit/native_shader_compiler1. Logs configure1/build1/build2 retain
commands/results; generated parser files stay in the build tree. Windows tar
could not locate xz initially; Python3.13 lzma/tarfile with safe extraction filter
successfully extracted the source. Archive/cache remain for reproducibility.

Next connect this proven compiler to original D3DXCompileShader semantics:
real profile/entry/flags/macros/include handling, API-owned output/diagnostics
copying and cleanup, native COM buffers and validated owned CTAB extraction.
Do not return synthetic constant data or mark compilation/initialization ready
before those adapters and original caller behavior are verified against SDK.
The full engine still needs wider audio/cinematic/UI/memory/game/renderer/network
services; this compiler probe does not substitute for production Com_Init.
