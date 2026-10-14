# Visibility and lighting source integration

2026-10-13. Actual full Com_Init remains incomplete.

Selected original DPVS, dynamic-model/entity/scene-entity visibility, spotlight
shadow, backend lighting, mesh-data and packing sources in the native database
library. These provide canonical visibility state, mesh helpers, lighting
sampling/patch allocation and packed-vector functions previously unresolved.
Moved five original effect event set/reset/wait bodies outside the CPU-query
guard in threads.cpp; their existing native backend owner creates the events.

Android DPVS debug formatting uses actual snprintf. Android backend lighting
now calls the existing owner-checked tlAtomicMutex::Unlock at two sites where
the decompiled code manually decremented the lock count and performed an atomic
operation on a temporary stack array. The same suspicious sequence exists in
BlackOps64, so Windows behavior is preserved; this is an Android integration
correction, not evidence of a different agent deleting the original lock code.
The canonical native Unlock releases its actual owner with full ordering.
This lighting path has not been exercised at runtime.

Native scene-entity culling uses the canonical R_CullSphereDpvs body in r_dpvs.cpp
instead of compiling its same-algorithm local duplicate. Under the compiler's
MS compatibility behavior, the prior external declaration made both copies
external, producing an observed duplicate symbol. Windows retains its local
definition. No successful return or diagnostic endpoint substituted.

## Evidence

Logs in build-android-audit/native_registry_closure:

- link48 preserves incorrect assumed source names; actual meshdata owner found
  and nonexistent lightgrid source removed (lighting resides in rb_light.cpp).
- link49 preserves Windows formatting and exchange compile errors.
- link50 preserves the duplicate sphere-culling symbol.
- link51 compiles selected sources and reaches remaining production symbols.

Nine whole affected Windows TUs pass x86 and x64 comparison in
build-android-audit/takeover_visibility_lighting_windows1. The subsequent change
only excludes the local sphere helper on Android; Windows branch unchanged.
No new device runtime checks or transfers occurred. Earlier mutex and queue
runtime results are retained with their original scope, not treated as lighting
or full-startup verification.

Continue from link51: model LOD, client/pose/demo, sound/cinematic,
physics/collision, further renderer state/material helpers, input/network and
recovery remain unresolved. Runtime source-size/atomic audits remain necessary
as dependency owners are integrated. Full startup/frame/zone loading unproven.
