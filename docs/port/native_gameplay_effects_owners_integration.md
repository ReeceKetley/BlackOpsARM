# Canonical gameplay and effects owner batch

Production source selection covers nine previously omitted owners:
`game/actor_function_table.cpp`, `game_sp/actor_sp_event_listeners.cpp`,
`game_sp/g_sp_vision.cpp`, `cgame/cg_scr_sp_blur_commands.cpp`,
`cgame/cg_effects_load_obj.cpp`, `flame/flame_cull.cpp`, `flame/flame_damage.cpp`,
`flame/flame_sound.cpp`, and `EffectsCore/fx_profile.cpp`.
The actor table additionally references the omitted direct owners
`game/actor_negotiation.cpp` and `game/actor_zombie_dog_exposed.cpp`; root selects
and integrates those. Other directly referenced actor state owners are already
selected. Existing Windows `Actor_SP_NotPorted_*` diagnostic rows remain intact.

Six assigned sources need no native algorithm change. Three have Android-only
ABI adaptations, preserving the original Windows branches and CP1252-compatible
ASCII source bytes:

- Raw impact loading allocates `sizeof(FxImpactTable)` with its native alignment
  and twenty-one actual `FxImpactEntry` rows. It addresses the original flesh
  fallback as `nonflesh[i][7]`: the decompiled negative index linearizes to
  `21*31 - 161*4 + 31*i = 31*i+7`. Surface type seven is flesh. No table format
  or fallback behavior is replaced.
- CSV filename sorting uses actual `const char*` array storage and a callback
  with the exact qsort signature. Flesh-name conversion likewise uses an exact
  signed-index callback wrapper around the canonical unsigned-index function.
  File-list terminators, signed allocation capacity, and actual received file
  lengths are checked before parsing; short reads close the file and retire high
  temporary memory. Token copying uses the actual token byte array.
- Flame sound copies the two actual float coordinates instead of aliasing them
  through a QWORD. Its existing stream path asserts actual render-list existence
  and a fuel-segment index inside the fixed 128-vertex array.
- Effects profiling uses exact qsort-signature wrappers around its canonical
  typed comparators; pointer-bearing entry sizes already use native sizeof.

All nine actual whole TUs compiled ARM64/API28 with UBSan and no diagnostic
startup/string-pool exclusions. Source logs are
`build-android-audit/native_gameplay_effects_owners/*-build1.log`; final native
list-storage correction is covered by the rebuilt fixture. No additional
source flags or associated header changes are required. Normal Windows comparisons
and expanded production link evidence belong to root integration.

## Focused impact-loader execution

`tools/port/tests/native_impact_effects_tests.cpp` includes the actual canonical
raw impact-loader TU, links actual `q_parse.cpp`, `surfaceflags.cpp`, and Android
legacy CRT, and supplies isolated real directory/file/hunk/FX asset providers.
The fixture FX provider returns typed immutable `FxEffectDef` test objects by
actual parsed name; unsupported names abort. It is explicitly a fixture asset
registry, not production FX loading. Temporary and permanent allocations use
real aligned storage with canaries; file input uses opendir/fopen/fread/ftruncate.
Malformed-list injection exercises the loader's bounded failure handling.

Build from BlackOpsARM:

```powershell
./tools/port/tests/build-native-impact-effects.ps1
```

NDK30.0.16248370 / API28 / ARM64 / C++20 / signed char / UBSan no recovery /
static libc++ / native MSVC spelling adapter / section garbage collection.
No engine-slicing definitions are used.

HONOR `AKWJ9X5116G04618` completed one complete impact-table fixture with exit0,
`IMPACT_EFFECTS PASS checks=2006` (including repeated allocation/retirement guards).
Checks establish native header/table allocation sizes and alignments; all735
published pointer entries; canonical CSV and surface parsing; file sort override
and flesh fallback; pointer lifetime after temporary arena retirement; surrounding
canaries and balanced parser sessions; real short-read cleanup and malformed-list
rejection. Build and HONOR logs are under
`build-android-audit/native_gameplay_effects_owners`.

Runtime evidence excludes real retail impact/FX loading, initialized engine FS/
hunk ownership, rendered effects, flame/audio consumers, profiling iteration/
concurrency, vision/blur consumers, actor state execution, Com_Init and gameplay.
Compilation and the fixture do not establish those milestones. Only the new
fixture binary was transferred; original assets and protected checkouts were
untouched.
