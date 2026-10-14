# Localization and asset-enumeration source integration

The active objective remains actual Com_Init and subsequent Com_Frame/headless
Zombies execution. This continuation integrates original localization and asset
enumeration sources. Full startup and their new runtime probes remain unfinished.
BlackOps64 is the validated behavior reference; its files were inspected read-only.

## Production changes and completed evidence

- Complete original stringed_hooks/ingame/interface/remote sources compile and
  are selected in the full native library. The prior selected hooks object is
  removed from entry_state to keep one canonical owner of language state.
  Three inherited unavailable endpoints are suppressed only for the full build
  so their real implementations are selected; no new placeholder was added.
- Original DB_EnumXAssets/LoadObj/For bodies and their canonical file-data table
  are selected from com_memory.cpp. The FastFile path is preserved. The previous
  four-argument call to a three-argument function and pointer-cast callbacks now
  use typed adapters with synchronous, stack-owned context. XAssetHeader bytes
  are transferred through memcpy with a native-size assertion. All original
  object/hash traversals are retained. This needs runtime callback verification.
- Image/material global declarations were moved once into canonical headers;
  their existing storage remains in the original source files. Offset/size
  assertions cover both pointer widths. No parallel tables or layout bypass.
- Original Image_IsProg uses a 30-slot address interval around a 29-element
  array. Integer address comparison preserves the reference interval without
  constructing an out-of-bounds array pointer or comparing unrelated objects.
- Original key-reading/formatting bodies are selected. Android uses actual
  engine text-file services. FILE retains the Windows CRT type, checksum bytes
  use memcpy rather than potentially unaligned/aliasing int access, and the
  overlapping formatting move now uses memmove. Missing/short-key policy remains
  the original behavior. No authentication/network action was added.
- Original CL_LocalClient_GetActiveCount and CG_IsShowingZombieMap return values
  are selected directly from their canonical source bodies. These are accepted
  intentional Windows reference stubs, not newly invented Android substitutes.
- Original I_strupr is selected; two malformed diagnostic path literals in
  com_memory.cpp are escaped correctly. Assertions remain active.

HONOR UBSan key-image-run1.log passes 50 focused checks in the existing filesystem
integration executable. New checks cover actual key IO, checksum bytes, overlap-
safe formatting, short/missing keys and program-image classification. Its key
fixture is synthetic, guarded against overwriting an existing file, and removed
afterward. All earlier filesystem/IWD checks still pass. No retail asset transfer
or modification occurred; only the changed binary was pushed and chmodded.

Eleven whole normal Windows TUs pass x86/x64 in
build-android-audit/takeover_localization_enumeration_windows1. This includes full
common/memory/localization, reference query sources and both image/material
owners with canonical layout assertions. This is compilation evidence, not
runtime equivalence of every changed path.

## Actual blockers and unexecuted tests

Full engine link30 still fails on 55 real services, especially resource events,
renderer/cinematic, input/screen/event processing, asset post-load and recovery.
Native static libraries build, but full Com_Init does not link/run yet.

The new bo1_native_filesystem_startup_tests calls actual FS_InitFilesystem with
real configuration prerequisites and takes existing game/user directories. Its
purpose is genuine mounted-asset/localization execution, not substitute Com_Init.
It has NOT run: build2 fails to link on the original lookup/resource/recovery
dependencies. Build1's test-header LARGE_INTEGER collision was corrected by
using startup_services_api instead of importing a competing global alias.

The new bo1_native_enum_localization_tests uses the original pool initialization
test source with an explicit extension selector for typed callbacks and StringEd
parser/package ownership. It has NOT run: enumeration-localization-build2 lacks
32 actual services. Fixture hash/table membership is explicitly restored in its
source; no real asset publication claim is made.

Selecting real localization lookup also means the unchanged standalone pool test
cannot currently relink: full-pool-build6 has the same resource/recovery closure.
Prior full DB_Init runtime evidence remains historical passing evidence (FACT153),
not proof that today's expanded graph is runnable. Keep this distinction visible.

Logs remain under build-android-audit/native_registry_closure. Native localization
syntax logs localization-0..3-compile1 pass. full-engine-link27 preserves duplicate
inherited endpoints, corrected by selecting real source ownership. Later failed
links, both startup builds and both enumeration builds are retained. No test
failure was hidden by introducing a successful or unavailable replacement.

Next implement the real resource/renderer/event and post-load dependencies from
full-engine-link30; preserve renderer-owned event initialization. Then relink/run
the pending original FS initializer and enumeration/localization probes, and
resume full production Com_Init. No full filesystem startup, real DB_Thread zone
loading, Com_Init return, Com_Frame or gameplay completion is claimed.
