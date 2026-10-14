# Physics and glass source integration

2026-10-13. Full Com_Init remains incomplete.

Full native library selects original physics main/collision/system, collision
geometry/GJK, glass client and curve owners. Physics object operations, narrow
userdata decoding, GJK shape construction, glass area/vertex functions and curve
construction now come from their actual engine sources. No dummy object, success
endpoint or simulated physics readiness introduced.

Native adaptations: physics diagnostic formatting uses snprintf; original
prefetch uses a real compiler primitive on Android; pointer alignment assertion
uses native uintptr_t without truncating the address; joint-axis row accesses
remain within their declared subarrays. phys_local.h includes the placement-new
declaration its templates actually use. Windows algorithm behavior unchanged.

## Verification

full-engine-link73 preserves native CRT/prefetch compilation failures. Link74
compiles initial sources,75 exposes missing placement-new include,76 compiles
expanded sources and reaches unresolved real dependency owners.

Seven whole Windows TUs pass x86/x64 in takeover_physics_glass_windows2. Initial
Windows1 fails the existing x86 GJK x87 requirement. The successful comparison
uses /arch:IA32 for x86, matching that source's documented vcxproj setting;
x64 ignores that option. No x87 guard weakened and no convergence limits changed.
The final include-only placement-new fix compiles natively; its declaration has
no algorithm change to the already checked Windows bodies.

No new tablet runtime test or transfer. Native physics initialization, joint/GJK
accuracy, glass execution and whole engine startup remain unverified. Portable
ARM/x64 float behavior is retained; source compilation does not establish
numerical parity with the Win32 x87 path.

Continue from link76: real rigid-body constraint methods, bgame/game/script state,
cinematic/sound, client/ragdoll/destructible, renderer helpers, input/network and
recovery still require their canonical owners. Preserve native/disk widths and
runtime ownership as those sources enter the build.
