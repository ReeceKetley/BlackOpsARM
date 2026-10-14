# Native render resource callback signatures

Updated 2026-10-14. Six typed callback registrations now use exact native resource
queue adapters. The four source files are frozen and released for SDK446/stage18.
This source validation does not establish a completed frontend frame or gameplay.
Root owns the linked APK and actual tablet integration result.

## Actual failure and queue contract

SDK445/stage17 has engine BuildId `02da0b621d137b9feff126f8a4dfd02e20044ac0`.
The genuine run passes the weapon option color rows and client initialization,
then UBSan reports an incompatible `void (*)(void *)` invocation in
`RB_Resource_Update_Internal`, `rb_resource.cpp:365`. The reported target
`0x37c29dc` resolves against the preserved stage17 ELF to
`XModelOptimizeCallback(XModel*)`, `r_xsurface_optimize.cpp:9`.

Raw evidence remains in `build-android-audit/native_registry_closure/`:
`sdk-native-entry445-follow1-only.log`, `sdk-native-launch445-follow1-crash.txt`,
maps and the dirty guard. The actual process was no longer alive at handoff.
SDK444's earlier visible engine loading UI remains the visual authority;
SDK445's subsequent initialization progress does not prove playable rendering.

`RB_Resource_CallbackParam` stores `void __cdecl (*)(void *)` and the unchanged
opaque data pointer. `ACTION_CALLBACKPARAM` invokes exactly that type. Casting
`void(XModel *)` to this type does not create a matching function definition.
The no-argument action independently stores and invokes `void __cdecl (*)()`.

## Scoped implementation

Each new static adapter is gated by `BO1_NATIVE_64BIT && !defined(_WIN32)`.
It takes `void *`, converts only the payload with `static_cast`, and directly
calls the existing canonical typed callback. The queue registers that adapter
without a function-pointer cast. Windows retains the original registration.

| Canonical callback | Original payload / conversion |
| --- | --- |
| `XModelOptimizeCallback` | `model`, `XModel *` |
| `XModelUnoptimizeCallback` | `model`, `XModel *` |
| `R_TakeScreenshotTgaCallback` | static `filename`, `char *` |
| `R_TakeScreenshotJpgCallback` | static `filename`, `char *` |
| `CL_CompositeSetupImageCallback` | `&job->resultImage`, `GfxImage **` |
| `UI_GenerateHeatMapTextureCallback` | `img`, `GfxImage *` |

All six canonical callback bodies remain byte-for-byte unchanged. Registration
conditions, data pointers, static filename ownership, result-image publication,
model optimize/unoptimize work, render-thread branches, flush calls, lock/order
and object lifetimes remain unchanged. The heatmap path remains under its
existing `BO1_HEATMAP` feature gate; that feature is inactive in production.
`rb_resource.cpp`, its header, providers and assets were not modified.

## Validation and limits

All four complete TUs compile under the actual Android shared-engine commands:
NDK30/API28, O0/debug/PIC, function instrumentation and UBSan with no recovery.
All four also compile under MSVC Windows x86 and x64 comparison configurations
in the Android worktree. These are 12 passing compile checks, with no linked
Windows runtime claim and no GPU mock fixture.

Production Android preprocessing across all 11 queue/registration TUs finds
16 active resource callback registrations and zero function-pointer casts.
Their callbacks match the corresponding exact `void()` or `void(void *)`
declaration. The existing conditional UI screenshot decompression callback is
already `void(void *)`; its `BO1_SCREENSHOT` registration is inactive in this
production configuration and remains unchanged. The independent render-command
composite MIP callback is outside the resource queue family and remains unchanged.

Windows x86 and x64 before/after preprocessing of all four TUs preserves all
nonempty text except diagnostic `__FILE__/__LINE__` metadata and profiler variable
names generated from `__LINE__`. Those metadata changes arise from source
relocation/inserted lines. The comparison normalizes only these explicit sites;
predicates, constants, normal variables, callback registrations and actual calls
compare exactly. Byte-level reversal of only the six guarded registrations and
six adapter additions recovers each exact pre-edit source. This before-edit
oracle includes earlier Android adaptations already present in the worktree.

Source encoding remains CP1252 and existing CRLF/bare-LF counts are preserved.
Hashes remained stable through compilation and the final audit. No sanitizer
weakening, device operation, shared master build, Gradle run, asset transfer,
provider change, commit or push was performed by this agent.

Evidence is in `build-android-audit/native_render_resource_callbacks1/`:
`source-manifest.json`, preserved pre-edit sources, `compile-results.json`,
`audit-results.json`, `signature-results.json`, `sdk445-target-addr2line.txt`,
native objects/preprocessed TUs, Windows objects/preprocessed comparisons and
all reproduction scripts/commands/logs. Initial audit harness corrections are
recorded in `audit-harness-corrections.txt`; they required no source edits.

## Frozen source hashes

| Source | SHA256 |
| --- | --- |
| `src/gfx_d3d/r_xsurface_optimize.cpp` | `8A6AE4129CE71D03C1A6A730CD7B491CF8D2437B1CF94A4C088487AC173A6F4A` |
| `src/gfx_d3d/r_screenshot.cpp` | `C196303B658BA89A37ADBEBDD726438E7119B37EA14208ACFC1D9A1D2B52D7C4` |
| `src/client/cl_compositing.cpp` | `7A175DFDF170B178EBD4E56A71D2E675B685019DF70DE2FC0B4AC6CA8FA01B7C` |
| `src/ui_mp/ui_main_mp.cpp` | `7F2D18C2036B67544C728AB12F30F7B28BFDA7486AF050651F54571C303EE9A4` |
