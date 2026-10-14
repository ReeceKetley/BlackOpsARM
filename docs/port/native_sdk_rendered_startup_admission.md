# Native SDK rendered startup admission

2026-10-14. Bounded source owner: `src/win32/win_main.cpp`, conditional controller header inclusion and `Sys_InitializeHeadlessPolicy` only. Configuration, controller adoption publication, runtime, Java, device execution and full linking belong to root/other owners.

The existing dedicated policy accepts exactly `s_headless && !s_headlessClient` as before. Under `__ANDROID__ && BO1_NATIVE_SDL_RENDERED_ENTRY` only, the formerly rejected branch now accepts `!s_headless && !s_headlessClient`, command-line `dedicated` value zero, and both actual `SDLJavaSetupComplete()` and `SDLActivityAdopted()` publications. Missing actual SDK adoption aborts explicitly. Mixed/headless-client or unsupported mode still aborts. With the flag absent the original rejection message/behavior remains. Root must define the flag only for the full native entry-state closure.

This is admission, not input/video/audio/device initialization or readiness. The new code neither publishes state nor invokes SDL provider startup. Genuine completed SDK Activity adoption is required; manual controller-Java setup alone is insufficient. The existing runtime separately validates the real SDK entry argument contract and accessible data roots.

## Canonical provider closure and retained gates

- `Sys_Init` (`win_main.cpp`) calls Android `InitializeSystemOS`, then actual `IN_Init`, then `BootstrapNativePlatform`. Native input installs the rendered translator/event consumer and Android console command owner through `InitializeRenderedInput`, then `GPad_InitAll`. Event owner identity, SDL events, completed Java setup and actual controller initialization remain required. The native registry's audio/render/video descriptors remain unbound; that descriptor registry does not establish readiness or replace canonical calls.
- `Com_Init_Try_Block_Function` (`common.cpp`) runs actual `CL_Init` for non-dedicated mode. Its client branch calls `R_InitThreads`, `CL_InitRenderer`, `SND_Init`, `CG_SndGameReset`, `Voice_Init`, and `R_Cinematic_Init`. Full renderer code is selected by the production per-source `-UBO1_NATIVE_RENDER_GLOBALS`; this policy does not bypass the branch or set engine ready flags.
- `renderer_window.cpp::ValidateActivity/PrepareOnOwner` requires the actual SDL owner, completed Java setup, valid JNI/Activity not finishing/destroyed, SDL video subsystem and real video driver. Display/window ownership requires actual display modes and dimensions, matching window lifecycle identity, valid non-hidden mode and a positive Vulkan drawable. Exact unsupported exclusive modes fail. `r_init.cpp::R_PreCreateWindow/R_CreateGameWindow/R_InitHardware` retains actual DXVK Direct3D interface, adapter/capability, device/resource creation and failure cleanup. Android headless window creation remains rejected.
- `faudio_bridge.cpp::IXAudio2::Create` requires actual SDL event ownership and Java setup before real FAudio construction. `SD_XAudio2EnumerateDevices/SD_Init` retains actual output-device enumeration/testing, master/submix graph creation, real `StartEngine`, and fatal failure handling. A compiled audio bridge does not prove a working output device.
- `voice_audio.cpp::JavaAndPermission/VoiceCapture::Open` additionally requires actual RECORD_AUDIO permission, live SDL audio and a real capture device. Canonical `Voice_Init` handles failure by retiring resources and leaving `g_voice_initialized` false; `Com_Init` intentionally ignores the optional voice return. No fake microphone grant is added.
- In the full engine, `tools/port/engine/unavailable_services.cpp` removes its early startup endpoints by `BO1_NATIVE_FULL_DATABASE`. The remaining `dwInit` endpoint is reached only with `BO1_LIVE`, outside the current configuration; it continues to fail closed if invoked.

## Final source checks

Whole Android TU compile passes with the current production command augmented only by `-DBO1_NATIVE_SDL_RENDERED_ENTRY=1` and a separate output path. Whole Android flag-off compile also passes. Normal Windows x86 and x64 each pass once using the established `/permissive /DWIN32=1` command contract. The source hash remained fixed during all four checks. Evidence is `build-android-audit/native_sdk_rendered_admission1/{compile-results.json,*-command.txt,*.log,source-manifest.json}`.

Only two byte replacements were made. All inserted lines are ASCII/CP1252-compatible CRLF; existing bytes and mixed newline history elsewhere are untouched. Removing the conditional inclusion and replacement policy restores the exact captured prechange file. Both changes are inside the native headless-selection region, with the new controller include and admission branch additionally Android/flag guarded; normal Windows bodies are unchanged. Existing working-tree changes before this assignment are preserved.

Before SHA256: `9470B67F24019B60EC50342E106140595027A68E93FE18FC742725FE698201BC`

Final SHA256: `A5A5847351B77E0C3DA4B60E2B08F488DF8395E673398AC4F4603A6C8B6A611C`

No runtime fixture manufactures adoption. No tests/device launch/full CMake build/asset transfer/ledger mutation occurred in this bounded assignment. Actual Activity startup, positive input, drawable/device creation, rendering, audio output and lifecycle teardown remain unverified until the real SDK APK executes them. Successful compilation alone proves none of these runtime outcomes.
