# Effect drawing and renderer source integration

2026-10-13. Com_Init and Com_Frame remain incomplete. This step verifies source
compilation and Windows compatibility, not graphics initialization or gameplay.

The full native library now includes original FX draw/marks/load-object/beam/
post-light/sprite sources, renderer model/warning/shader-constant sources and the
existing Android region-tagged Narrow32 owner. Missing effect/trail handle and
cooperative iterator implementations now come from their original owners.
No replacement services or fake readiness were introduced.

## Native fixes

- Cloud drawing previously cast a four-byte counter to an eight-byte atomic
  pointer. Android now updates the actual unsigned32 field using the existing
  full-barrier atomic overload. The original draw API's const view is explicitly
  removed for this mutable runtime counter, matching its previous write intent.
- Shader frame and ready stamps have one canonical declaration, four-byte width
  assertions and Android atomic storage. Ready-stamp publication after copying
  constants uses sequentially consistent store, with atomic reader loads and
  frame increments. Windows retains volatile LONG and its original Interlocked
  publication.
- Renderer warning formatting uses actual native va_list/vsnprintf semantics;
  va_end is paired with va_start. Shader debug formatting uses native snprintf.
- Trail segment copies index the second basis row directly, removing array
  subobject bounds violations. Small size/color copies use memcpy rather than
  potentially unaligned/type-punned double/int accesses, preserving bytes.
- Six original mark prefetch calls use a native compiler prefetch on Android;
  Windows retains its original intrinsic/hint.

## Evidence

Logs under `build-android-audit/native_registry_closure/`:

- link41 preserves native formatting and const-counter compilation failures.
- link42 compiles the first source expansion and reaches unresolved services.
- link43 preserves six unsupported mark prefetch intrinsic errors.
- link44 compiles all selected sources and fails to link on 101 unresolved
  production symbols. This count describes the current expanded dependency
  graph, not remaining effort or a completion percentage.

Normal Windows x86/x64 whole-source checks pass:
`build-android-audit/takeover_effect_draw_windows1/` (five TUs) and
`build-android-audit/takeover_effect_primitives_windows1/` (six TUs, including
the final trail-copy changes). Source manifests and build-result files accompany
both runs. The existing native Narrow32 implementation was selected, not changed.

No new runtime gate executed and no assets or binaries were transferred.
The shader publication/cloud/trail paths await meaningful runtime exercise once
their actual dependencies link. Prior passing filesystem/queue evidence is not
repeated or promoted to a full startup claim.

Next source owners include renderer code-mesh helpers, client state/pose,
sound/cinematic, marks/physics/collision, networking/input and recovery. Continue
from full-engine-link44.log, preserving genuine hardware/resource behavior and
auditing fixed-width/runtime-layout assumptions as those sources are selected.
