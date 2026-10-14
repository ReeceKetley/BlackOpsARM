# Native vertex color pack alignment

SDK440 / APK12 (engine BuildId `4c3105fa41f9181758eded101d5f26de337887ed`) genuinely returns Com_Init and reaches frontend loading UI painting. Its next UBSan abort is `fx_convert.cpp:1582`: Byte4PackVertexColor reads a `uint64` from real float color address `0x006f0ad7a204` (four modulo eight). The matching actual stack passes Window_Paint, UI_DrawHandlePic, CL_DrawStretchPic, R_AddCmdDrawStretchPicW and R_ConvertColorToBytes. Evidence: `build-android-audit/native_registry_closure/sdk-native-entry440-launch1-only.log` and the matching SDK440 crash log. This failure is independent of shader/GPU readiness.

## Contract and repair

Byte4PackVertexColor accepts four floats through `const float*`, guaranteeing only float alignment (four bytes). `WindowDef` foreground/background/border fields are declared `float[4]`; the actual crash confirms an allowed four-modulo-eight instance. FX_SampleVisualState also passes local `float rgba[4]`. The destination is `unsigned __int8*` owning four bytes, with no required integer alignment. Real callers include GfxColor command destinations through R_ConvertColorToBytes and `FxElemVisualState::color[4]`.

The Android native-width branch now copies the exact 16 input bytes to the existing aligned float4's actual `v[4]` storage with memcpy, then copies the final original four-byte `permed.u[0]` representation to the destination with memcpy. No arithmetic, float constants, clamp comparisons, floor/bias behavior, integer conversion, channel permutation, alpha or gamma behavior changes. The existing permutation bytes are `0c000408`, selecting B/G/R/A from the original integer lanes. No structure packing, asset change, assert bypass, sanitizer suppression or graphics provider change.

All Windows original lines remain in their original branches. Branch-selected Byte4PackVertexColor matches the preedit function and the read-only BlackOps64 oracle exactly. Source remains CP1252 with CRLF. Only `src/EffectsCore/fx_convert.cpp` changed.

## Same-family audit

`r_state.cpp::Byte4PackPixelColor` already reads `from[i]` as floats and writes `to[order[i]]` as bytes, retaining its existing I_RoundBiasToInt and B/G/R/A order. `cg_scr_main.cpp::Byte4PackRgba` likewise uses scalar float/byte accesses and its own existing truncation/bias contract. Neither needs a wide-copy repair; both remain untouched. No other float-to-byte color pack function exists in fx_convert.cpp. Its other `_QWORD` copies copy curve pointers in FX_DecideVelocitySampleCount, a different pointer-width/graph family outside this repair.

## Validation and bounded fixture

- Actual production ARM64 compilation command retains O0/debug/PIC, function instrumentation, UBSan and no-recovery: full fx_convert TU exit0.
- Normal Windows x86 and x64 full fx_convert TUs exit0. The read-only r_state full native/x86/x64 TUs also exit0; no r_state edit.
- Native `vertex_color_fixture` links the actual full fx_convert native object and calls its actual Byte4PackVertexColor. Only instrumentation hooks are supplied by the fixture, explicitly `no_instrument_function`. Its sole production owner state, NATIVE_VERTEX_PERM, uses the actual TU's ordinary constant initialization. No fabricated engine/graphics globals or providers.
- Windows golden executable selects the exact function and NATIVE_VERTEX_PERM initializer from current canonical source, with actual q_shared/com_math declarations. It does not reimplement packing arithmetic. Selection is independently verified against preedit and BlackOps64. Both Windows x86/x64 golden runs exit0 and their 777 COLOR records agree exactly.
- 107227 checks per run cover 777 finite asymmetric/clamp/color vectors, all 256 rounding-midpoint lanes with predecessor/midpoint/successor floats, eight-aligned reference input, actual declared float-array input at four modulo eight, output byte offsets 0..7, original source bits and untouched exact four-byte output canaries. A known asymmetric vector independently verifies BGRA bytes `4D331A66`.
- Root executed the native fixture once on the actual HONOR: exit0 in less than one second, PASS 107227 checks. All 777 actual native COLOR records match the Windows x64 golden exactly (also identical to x86). Evidence: honor-fixture1.log and honor-compare-result.json. This establishes CPU packing contracts; genuine frames, visible content and gameplay remain unverified. SDK441 integration remains the runtime authority.

Windows whole-TU fixture linking could not close unrelated common.h netfield dynamic initializer/FX imports; failure logs retained. A first canonical-source golden compile omitted the actual q_shared declaration of `_QWORD`; those failure logs are retained as `*-golden-attempt1-compile.log`. Adding the actual declaration header fixes the fixture without editing production source or arithmetic.

Artifacts: `build-android-audit/native_vertex_color_pack1/` contains before source, source/audit/compile manifests, full compile commands/logs, exact selected Windows function, fixture source/build commands, Windows golden outputs and the native executable. Compare native COLOR lines against `x64-golden.log` (also identical to x86-golden.log).

Frozen fx_convert SHA256: `DA5F46F476443371C9375367D52DA1D0B922C1A207D94B74B4D0A8F6BC8446A5`.
Frozen native fixture SHA256: `9662E74FE2A4550231DD99E8CA1483426863396FA332C168DE644D406A97280B`.
