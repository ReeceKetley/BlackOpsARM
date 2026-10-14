# Native server movement collision visitor lifetime

2026-10-14. SDK450 server failure identified from its preserved matching ELF; source frozen for SDK451 integration. Actual SDK451 movement execution remains unverified by this report.

## Actual failure and producer proof

`build-android-audit/native_registry_closure/sdk-native-launch450-follow1-crash.txt` contains historical crashes followed by the actual SDK450 SERVER failure: PID12927, tid13085, SIGSEGV/SEGV_MAPERR, fault address0x30. Only the latest matching segment is used here. Preserved `build-android-audit/apk-native-stage22/arm64-v8a/libblackopsarm_engine.so` has BuildId3850c57b9fa3ab40d4ca7b47098475224bed6919, exactly matching this crash. NDK30 LLVM symbols resolve PC0x237835c to `phys_gjk_collision_detection.cpp:1193`, the virtual `input.m_proximity_data->update(...)` call.

The matching instructions are:

```text
0x2378358: ldr x8, [x0]
0x237835c: ldr x8, [x8, #0x30]
0x2378364: blr x8
```

Crash x0 is0x6f124854d0, x8 is0. This is a live visitor address whose virtual-table pointer has been erased, so loading its update slot at0x30 faults. It is not a null `m_proximity_data` pointer.

The matching ELF `g_pmove` symbol is0x7a69270. Crash absolutePC0x6f0cd9435c minus relativePC0x237835c gives loadbias0x6f0aa1c000. Native record layout puts `pmove_t::proximity_data` at608/0x260. Therefore:

```text
0x6f0aa1c000 + 0x7a69270 + 0x260 = 0x6f124854d0
```

This equals crash x0 exactly and identifies `g_pmove[0].proximity_data`. The stack proceeds through cached-query resize, gjk_trace, gjk_slide_move, PM_SlideMove, PM_FlyMove, Pmove, SpectatorThink and server ClientThink/UserMove. `setup_gjkcc_input` publishes the address of this embedded visitor; the query carries it through unchanged.

The global `g_pmove` objects are constructed normally. `pmove_t` constructs its embedded `colgeom_visitor_inlined_t<200>`; that constructor installs the real virtual table through ordinary C++ construction and resets the query cache. The Android SpectatorThink producer then called `memset(pm,0,sizeof(*pm))`, erasing all3968 native bytes including the3344-byte polymorphic visitor and its virtual-table pointer. ClientThink_real had the same destructive Android reset. Validated BlackOps64 clears only the ordinary movement prefix0x258 and preserves the visitor/cache. Windows x86 layout places the end of its m_gjkcc_input pointer at600/0x258; an eight-byte alignment gap precedes the visitor at608. Android's pointer ends at608 and its visitor begins there.

## Repair and bounded scope

Only `src/game_mp/g_active_mp.cpp` changes, at SpectatorThink and ClientThink_real. Under the existing Android guard, both reset sizes now come from the typed address of `pm->proximity_data` relative to `pm`, clearing the native608-byte ordinary movement prefix. The constructed visitor, virtual table, traversal state and cached primitive data survive exactly as the canonical Windows movement lifecycle requires. The appended ordinary `spDisabledStances` field is explicitly reset to0, preserving the prior Android full-clear behavior; ClientThink_real retains its subsequent canonical Zombies permission assignment.

The original Windows0x258 reset remains byte-for-byte unchanged. No visitor replacement, reconstruction, fake collision result, query suppression, algorithm/vtable change, assertion relaxation or UBSan change is introduced. GJK sources and cg_scr_main.cpp remain untouched.

A bounded search of cg_predict_mp.cpp/.h, g_active_mp.cpp, bg_pmove.cpp, bg_slidemove.cpp and g_sp_headless_move.cpp found only these two whole-pmove reset producers. Other matching copies operate on usercmd fields. The existing pmove constructor uses typed visitor reset and preserves construction. No wider movement repair is inferred.

## Verification and frozen source

Evidence directory: `build-android-audit/native_server_pmove_visitor_lifetime1`.

- Actual native whole g_active_mp.cpp compiles using the production compile_commands.json, NDK30/API28, O0/PIC/debug/instrument-functions/UBSan-no-recovery configuration; exit0.
- Whole normal Windows x86/x64 TUs compile in BlackOpsARM using the established MSVC comparison commands; both exit0. BlackOps64 is read only.
- Native before/after preprocessing checks both selected bodies and proves the only active changes are the movement-prefix reset and explicit appended stance reset. Existing macro-generated source-line metadata is normalized; handwritten assertion locations are retained.
- Windows x86/x64 selected bodies compare identical before/after. Read-only BlackOps64 preprocessing independently confirms the original0x258 clear in each corresponding producer. Existing unrelated Windows feature additions prevent a claim of full oracle-function equality.
- Native and both Windows layout fixture compiles pass. Native generated assembly calculates a0x260 movement prefix and writes the appended stance field separately. Layout/nm/crash arithmetic proves exact producer pointer identity.
- CP1252 high bytes preserved. Existing CRLF endings preserved; before/after bareLF counts both0. The source snapshot remains unchanged after checks.

Frozen g_active_mp.cpp SHA256:

```text
before 8C0B617C4463F5862519DF5FDCD67DC098DC11ED56661743A524D413FF06944A
final  7145C5AA3E2FA67762FA89989476F8002895B2270E46A78C93B95B59179679A6
```

Source size125721→126133 bytes. `source-manifest.json`, `source.diff`, `check-results.json`, preprocessed selected bodies and compiler logs preserve verification. `matching-buildid.txt`, `server-symbolized.txt`, `crash-disassembly.txt`, `g_pmove-symbol.txt`, `pointer-producer-proof.json` and isolated `sdk450-server-crash.txt` preserve the direct crash evidence.

The initial fixture run successfully compiled all three whole production TUs but had two checking assumptions wrong: native __int8 preprocesses to char, and Windows0x258 ends before its visitor alignment gap. Those harness assumptions were corrected without another source change; the initial result is preserved in `check-results-initial-fixture-error.json`. The final complete check passes.

No full engine build, APK staging, device access, asset transfer or runtime-success claim is made by this source owner. Root integrates this frozen source with the separately owned CScr repair and observes actual SDK451; only genuine post-repair movement execution can establish runtime progress.
