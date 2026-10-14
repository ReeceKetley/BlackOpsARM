# Native geometry counter lifetime

The SDK468/stage40 failure is a signed statistics-counter overflow in the real 3D render path. The repair resets the actual embedded per-view accumulator at the existing canonical view reset. Production is frozen for SDK469/stage41.

## Actual failure

`native_registry_closure/sdk-native-entry468-console1-only.log` reports `rb_tess.cpp:1177:50`: `2147474811 + 15540` cannot be represented by `int`. The matching fresh crash portion of `sdk-native-launch468-console1-crash.txt` identifies PID10472, render thread10586 (`bo1-render`), BuildId `16169f5dc9777d411dff30bfa34da49cabf88ae7`, and process uptime307s. Its stack is `R_DrawXModelSkinnedUncached` → `R_TessXModelRigidSkinnedDrawSurfList` → `R_RenderDrawSurfListMaterial` → `R_DrawSurfs` → `R_DrawSpotShadowMapCallback` → `R_DrawCall` → `R_DrawSpotShadowMapArray` → `RB_SpotShadowMaps` → `RB_StandardDrawCommands` → `RB_Draw3DInternal` → `RB_Draw3D` → `RB_RenderCommandFrame`. Earlier crashes are also present in that buffer; they are not this failure. Observation happened during a console-key experiment, but neither the failing producer nor stack establishes a console cause.

## Ownership and reset contract

`GfxFrameStats` in `rb_state.h` is a636-byte runtime statistics structure: two view records and three signed32-bit totals. `geoIndexCount` is at byte628. It is embedded by value in `GfxCmdBufPrimState`; the separate `g_frameStatsCur` global has different storage. No header, layout, fixed disk/VM/wire slot, or count type changes.

The precise producer assigns `args.triCount = XSurfaceGetNumTris(xsurf)`, then adds `3 * args.triCount` to `context.state->prim.frameStats.geoIndexCount`. The observed addend15540 is5180 triangles. The overflow is the addition to a near-INT_MAX prior accumulator; the multiplication in this observation is representable.

`R_InitLocalCmdBufState` copies the whole persistent `gfxCmdBufState` into the local draw state. `R_DrawCall` invokes the callback on that local state, then publishes the whole local state back to `gfxCmdBufState`. Thus the embedded count survives local-state destruction and is inherited by the next draw, view, and frame. The existing `R_InitCmdBufState` initialization clears it at initial state creation, but the ordinary local-state initializer only copies it.

`RB_Draw3D` unconditionally calls `RB_ResetStatTracking` immediately before `RB_Draw3DInternal` in both the ordinary-view and missile-camera loop branches. Previously that function updated maxima and cleared only `g_frameStatsCur`; it never cleared the producer's embedded storage. Source reads establish a missing owner reset, not an intentional wrap contract: these are per-view/frame statistics, the existing per-view reset marks their lifetime boundary, and consumers display signed counts. No authoritative intentional32-bit wrapping contract was found. The read-only Windows oracle has the same producer, whole-state copy/publication, and reset behavior; its source does not supply a different reset of the embedded owner.

## Change

Only `RB_ResetStatTracking` in `src/gfx_d3d/rb_backend.cpp` changes. Under `defined(__ANDROID__) && BO1_NATIVE_64BIT`, it now calls `memset(&gfxCmdBufState.prim.frameStats, 0, sizeof(gfxCmdBufState.prim.frameStats))` at the existing reset boundary. All embedded statistics are reset together rather than clearing only the first observed overflowing member. The operation affects exactly636 bytes and preserves adjacent device, stream, shader, pointer, render-target and state fields.

The existing global reset, maxima call/order, view-index selection, and graphics/performance instrumentation remain intact. This repair does not rewire the preexisting separate global statistics consumers. No wrapping, clamping, assertion bypass, sanitizer weakening, new render scheduling, or draw-geometry change. `rb_tess.cpp` is unchanged and retains the earlier typed particle2x2-axis repair.

## Focused verification

- Native ARM64 production TU and Windows x86/x64 production TU compile successfully. Eight before/after/oracle preprocessing commands succeed using the actual native compile recipe and established Windows templates; all outputs stay in this Android worktree.
- Native reset-body comparison permits only the one owner-clear statement. Both Windows reset bodies are identical to their before versions and the read-only Windows oracle. Five adjacent frame/Present functions are unchanged in all three configurations after normalizing only compiler-generated assertion source file/line locations. Raw location-only comparison failures are preserved in `generated-file-location-raw-results.json`; the verified inverse-byte proof independently ensures no production edits outside the six-line insertion.
- `owner-results.json` proves both reset-before-draw branches, the exact whole-state copy, publication, and observed signed producer. All four source bodies are identical to the Windows oracle. No full renderer audit was performed.
- Source-extracted fixture uses real canonical headers/types, the exact reset and local initializer, the exact observed producer statement, and the exact publication statement. It compiles for actual ARM64 and runs on the Windows64 host. `fixture-run.log` records600010 checks across200000 views. Tests establish distinct owner/global addresses, old reset leaving2147474811 intact, the mathematical negative control2147490351 > INT_MAX, full-state byte containment, global maxima/reset order, stats view-index selection, two local draw copy/publication cycles within one view, and independent large per-view totals9015540 without cumulative growth. Frame-stat size/field-offset assertions pass in both compiled configurations; the host real state is5248 bytes, prim offset208, embedded stats offset104.
- The initial host fixture link required unused NetField metadata definitions from an included header. The first link log is retained. Fixture-only definitions use the exact declared array types and are never exercised; no production metadata/provider/stub change. Final host compile/run and native fixture compile all exit0.

These checks prove source behavior and bounded ownership/lifetime arithmetic. They do not establish a post-repair actual APK long-run result, every conditional rendering path, or playable Zombies. Root owns the SDK469/stage41 build/device validation and Ledger/STATE.

Root subsequently integrated SDK469/stage41, BuildId
`fdb9bc2173bccb513d8a94f4edb158e435cc899a`, with all seven packaged providers
matching their staged bytes. Actual touch OK passed the title into visible room
geometry with fresh FPS/CPU/GPU/RSS, and main4601 frames were reached without the
prior geometry-counter overflow. This process later aborted in a separate
console-name autocomplete callback while typing a command, before its execution.
The matching latest evidence is `native_registry_closure/sdk-native-entry469-console-map-text1-only.log`
and `sdk-native-launch469-console-map-text1-crash.txt`; actual room captures are
`sdk469-screen-touch-enter1.png` and `sdk469-screen-touch-menu1.png`. This proves
progress beyond the observed counter failure, not every conditional draw path
or playable Zombies.

## Frozen source and evidence

`rb_backend.cpp` SHA256 `6B368DE11E2423B105D17A975D9CCC4995E09551EE0228ECBC8DBEC37F1091EC`.

Unchanged `rb_tess.cpp` SHA256 `98693868F045AD8C9C09380A2A1F6E0F01C25F537D2CC0A84E8A295299525AC9`.

Evidence: `build-android-audit/native_render_geometry_counter_lifetime1/`: baseline source, source.diff, check.py, native/Windows commands/logs/objects/preprocessed bodies, check-results.json, owner source excerpts/results, lifetime-fixture.cpp/fixture.py/fixture-results.json/fixture-run.log and source-manifest.json. Source CP1252 high bytes and preexisting line-ending form are preserved. Report is written through a verified UTF8 temporary file.
