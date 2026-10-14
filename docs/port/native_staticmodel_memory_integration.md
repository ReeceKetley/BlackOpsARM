# Native canonical static-model memory owner

2026-10-13. Select src/gfx_d3d/r_staticmodel.cpp, with existing normal native
flags and no new selector. r_staticmodel.h is unchanged. Source is frozen;
prior renderer10 sources remain unchanged. Root owns CMake/diagnostics and normal
Windows comparison. No protected files/assets/ledger/commits changed.

The original owner R_StaticModelGetMemoryUsage enumerates each cell's root AABB
model-index references and matches XModel identity. Native preserves this policy,
including counting repeated references across/within cells; it does not change
the metric to unique-model counting or iterate child trees again. Its original
no-world result remains zero/count0. Real empty cells have no root to inspect.

The original 76-byte constant describes the Windows runtime draw instance.
Native R_StaticModelGetMemoryUsageInst uses actual sizeof(GfxStaticModelDrawInst),
80 bytes in this ARM build. It changes runtime accounting only, not any fixed
asset/file format. Native checks output ownership, negative cell/tree counts,
nonnull arrays required by positive counts and every root model index against
actual world.dpvs.smodelCount. It checks the int output arithmetic before adding,
and publishes modelCount after the complete traversal. Fatal callbacks abort if
they return; malformed data cannot become a successful memory report. Windows
original bodies/constants remain unchanged and source remains CP1252.

The caller still needs a stable borrowed rgp.world and asset-owner lifetime during
this command, as in the canonical original; no new lease/readiness is invented.
Reported lengths cannot prove externally supplied arrays actually own that
storage. This is not loaded FastFile/world, DB publication or GPU-runtime proof.

Evidence under build-android-audit/native_registry_closure:

- r_staticmodel-native-audit1.log and saved command JSON: production Android
  full source, instrumentation and UBSan compilation PASS.
- r_staticmodel-native-{defined,undefined}.txt: actual two canonical owners and
  dependencies; no test exclusion or substituted engine implementation.
- renderer-staticmodel-frozen-hashes.json: source/header/durable fixture hashes.
- renderer-staticmodel-fixture-build1.log/command.json and run1.log: HONOR
  AKWJ9X5116G04618 UBSan PASS, no recovery.

Durable tools/port/renderer_staticmodel_fixture.cpp consumes the actual canonical
source with valid fixture-owned world/cells/root/index/instance/model storage.
It verifies actual80-byte accounting, target identity, three original repeated
matches versus one other-model match, no matches, empty cell and absent world.
Six malformed input children test null output, negative cell count, absent root,
missing index array, out-of-range index and absent instance array. The isolated
fatal callback deliberately returns; all six children must receive SIGABRT before
returning a misleading result. It does not mock successful GPU/asset readiness.

Only this new79,536-byte fixture binary was transferred; no libraries/assets:

    adb -s AKWJ9X5116G04618 shell /data/local/tmp/renderer_staticmodel_fixture

No actual initialized model-list command, loaded world or Com_Init execution is
claimed. Continue production source integration and real startup.
