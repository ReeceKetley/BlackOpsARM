# Collision and dynamic-entity source integration

2026-10-13. Full Com_Init and gameplay remain incomplete.

Full native library now selects original cm_trace/mapkit/mesh/tracebox/test/world
and DynEntity load-object/client/collision sources. Native entry's previously
selected cm_trace state is removed only in the full build, ensuring one actual
clip-map query/trace owner. OFF diagnostic selection remains unchanged.

Five collision prefetch sites use native compiler prefetch on Android and their
original Windows hint. Mapkit's aligned brush allocation/free calls a real
POSIX platform service with arbitrary-size aligned backing and validated
alignment. New Android failure handling frees all partially allocated brush/
model/node arrays, clears their unpublished pointers and reports failure before
copying. Reference Windows allocation behavior remains unchanged.

## Evidence and scope

Native compilation passes. Nine whole normal Windows TUs pass x86 and x64 in
takeover_collision_dynent_windows1 (five) and collision_helpers_windows1 (four).
The subsequent allocation-failure branch is Android-only; Windows code unchanged.
full-engine-link65 preserves missing prefetch/aligned CRT compile errors;
link66/67 compile and reach actual remaining dependency symbols.

No new tablet probe or transfer occurred. Native aligned allocation/error
cleanup and the expanded collision paths have not been exercised at runtime.
No collision correctness, mapkit loading, physics state, startup or frame claim
is inferred from compilation. No new diagnostic endpoint or success substitute.

Continue from full-engine-link67: actual skeleton/IK, physics object/GJK,
cinematic/sound, glass, client state, input/network and recovery owners remain.
Preserve native pointer/disk-slot widths and audit original resource lifetimes
when selecting these real dependencies.
