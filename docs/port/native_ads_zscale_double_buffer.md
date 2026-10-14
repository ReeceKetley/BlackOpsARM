# Native ADS Z-scale double buffer

Updated 2026-10-14. `r_adszscale.cpp` is frozen and released for SDK448/stage20.
Root owns the next linked APK and actual tablet integration. This source repair
does not establish a completed frontend 3D frame or playable Zombies.

## Actual failure and caller audit

SDK447/stage19 engine BuildId `01bff569d01b457e81f90929ed374d0d8a8d428b`
passes the sound matrix repair, reaches CA_ACTIVE and processes the initial
snapshot. It then fails at `R_SetADSZScale`, `r_adszscale.cpp:16`, with index 1
outside the declared `float[1]` row. Raw evidence remains in
`build-android-audit/native_registry_closure/sdk-native-entry447-follow1-only.log`
and its corresponding crash capture. Actual process 28456 was dead with a dirty
guard; USB stayawake was restored to 0. The recorded loading UI metrics remain
loading/render submission observations, not a completed gameplay result.

The canonical declaration is `float adsZScale[2][1] = { { 1.0 }, { 1.0 } };`.
The previous producer indexes `[viewport][(activeAdsZScale + 1) % 2]`, and the
consumer indexes `[viewport][activeAdsZScale % 2]`. The allocation contains two
frame slots for one client, while these expressions reverse those dimensions.
Viewport 0 with parity 1 happens to address the second physical float in the
Windows build, but crossing the first one-element row is not a valid C++ access.
The pre-edit file exactly matches the read-only BlackOps64 reference.

The complete source search found one producer call in `CG_CalcViewValues` and
five shader constant calls: one in `rb_depthprepass.cpp`, one in `rb_draw3d.cpp`
and three in `r_draw_lit.cpp`. Every call passes `localClientNum`, rather than a
render view ordinal. `MAX_LOCAL_CLIENTS` is canonically 1 in `cg_local_mp.h`.
`CG_CalcViewValues` sets `refdef.localClientNum`, which is copied through
`sceneParms->localClientNum` into `viewInfo->localClientNum` for all consumers.
The header adds no alternate dimension or external data declaration.
`RB_StandardRenderCommands` remains the sole frame-advance caller.

## Narrow native indexing repair

Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, the producer now writes
`adsZScale[(activeAdsZScale + 1) % 2][viewport]`. The constant consumer reads
`adsZScale[activeAdsZScale % 2][viewport]`. The original array declaration,
initializer and eight-byte storage remain unchanged. Native static assertions
check two frame rows, one client per row and eight total bytes. Existing engine
`iassert` checks the unsigned viewport against that declared one-client extent
before either native access.

The frame counter increment, producer/read parity expressions, shader constant
identifier and remaining three zero components retain their original behavior.
No counter reset, wrap policy, publication order, threading, ticking, storage
growth, ABI, constant layout or sanitizer policy changed. Windows keeps both
original bodies and indexing expressions under the existing platform branch.

For the only valid viewport, 0, row-major physical offsets are identical:

| Parity | Previous physical offset | Native valid row offset |
| --- | --- | --- |
| 0 | `4 * (0 * 1 + 0) = 0` | `4 * (0 * 1 + 0) = 0` |
| 1 | `4 * (0 * 1 + 1) = 4` | `4 * (1 * 1 + 0) = 4` |

Thus the native producer and consumer use the same two floats and frame parity
as the validated single-client Windows behavior, while each native indexing
operation stays within its declared row. Both initial slots remain 1.0.

## Validation and freeze

The complete production TU compiles with its actual Android shared-engine
command: NDK30/API28, O0/debug/PIC, function instrumentation and UBSan with no
recovery. Windows x86 and x64 comparison compiles also pass, in the Android
worktree. No shared master build, Gradle, device operation or GPU fixture was
performed by this agent.

Actual native preprocessing confirms only the transposed producer and consumer
expressions are active, and the original declaration and initializer remain.
Windows x86 and x64 before/after preprocessed text compares exactly after removal
of empty lines and line directives, without expression or metadata
normalization. Reversing only the four scoped edits recovers the exact pre-edit
bytes and read-only Windows reference. CP1252 bytes and existing CRLF/bare-LF
counts are preserved. The frame-advance body remains byte-for-byte unchanged.
The algebra audit covers both parities and all valid clients.

Evidence is in `build-android-audit/native_ads_zscale_double_buffer1/`:
`source-manifest.json`, the preserved pre-edit file, `compile-results.json`,
`audit-results.json`, `all-call-sites.txt`, native and Windows objects and
preprocessed files, plus reproducible edit/compile/audit scripts and command
logs. No asset/provider modification, source changes outside the owned file,
commit, push or Ledger/STATE mutation was performed by this agent.

Frozen `src/gfx_d3d/r_adszscale.cpp` SHA256:
`5460A25A1B5AE5051B59CBBDDD32A69F096B6945199C21072D54ABB3392B8625`.
