# Renderer mesh and worker source integration

2026-10-13. Full Com_Init remains incomplete.

Selected ten original renderer sources in the full database library: drawsurf,
scene, common worker commands, DObj/model skinning, model pose/lighting, static
DPVS, mark fragments and water simulation. These provide real code/mark mesh
reservation, effect worker dispatch and other previously missing implementations.

Native fixes preserve actual behavior: cull-state CAS uses its declared four-byte
field, alignment attributes attach to objects, model-lighting minimum uses the
standard template, native scene formatting uses snprintf, and DPVS casts use
the same canonical uint64 type as their declared pointers. The DPVS change
resolves LP64 type identity; it does not certify existing type-punned accesses
throughout that decompiled algorithm as safe. r_model_pose.cpp's existing
Windows-1252 encoding was preserved for the narrow edit.

Android water updates now use a process-owned recursive pthread mutex in
src/platform/android/water_mutex.cpp. Initialization validates all OS results;
lock blocks on actual contention and returns failure on invalid handles; unlock
rejects non-owners. The original engine initialization/lock/unlock call sites
select this service on Android. Windows retains CreateMutex/Wait/Release.
This is the water lifecycle's infinite-wait anonymous mutex, not a general named,
finite-wait or abandoned-owner Win32 mutex compatibility implementation.

## Validation

- full-engine-link45/46 preserve initial compiler failures; link47 compiles
  these selected sources and still fails at remaining production dependencies.
- Ten complete Windows translation units compile for x86 and x64 in
  build-android-audit/takeover_renderer_mesh_windows1, with manifests/results.
- Actual native mutex service passes HONOR UBSan test from
  tools/port/tests/native_water_mutex_tests.cpp. The test covers recursive
  ownership, another thread waiting through one recursive release, acquisition
  after final release, non-owner/unowned rejection, null failure and join.
  Evidence: native_registry_closure/water-mutex-build1.log and
  water-mutex-run1.log. Only the 1.45 MB test binary was transferred; assets reused.

The complete water simulation, mesh rendering and broader source expansion have
not run on the tablet. No new placeholder endpoint or readiness bypass added.
Continue source closure from full-engine-link47.log: DPVS globals/helpers,
client/pose, light sampling, physics/collision, sound/cinematic, input/network
and recovery remain unresolved. Compilation is not successful initialization.
