# Native physics initialization and arena ownership

Scope: canonical `Phys_Init`, `phys_memory_manager_init`, permanent arena allocations and sweep-and-prune alignment. No physics algorithms, capacities, public headers, production source selection or assets changed. Original Windows bodies remain under their existing branch; CP1252 and original line endings retained.

Actual ARM64 measurements on HONOR AKWJ9X5116G04618:

| Type | Size | Alignment |
|---|---:|---:|
| phys_memory_manager | 1472 | 8 |
| physics_system | 25536 | 16 |
| broad_phase_memory | 6488 | 8 |
| axis_aligned_sweep_and_prune | 72 | 8 |
| phys_surface_type_info | 16 | 4 |
| phys_slot_pool / extra_info | 32 / 16 | 8 / 8 |

Canonical startup permanent topology is manager + physics system + broadphase + SAP + 630 surface cells: 43,648 bytes before any additional padding. The existing TL 16-byte-aligned 3,670,016-byte allocation suffices; it was not enlarged. Dynamic gameplay use has not been certified by this initialization audit. Slot footer alignment does not require changing encoded strides: fresh slot allocations already use the actual alignment individually.

Changes:

- Native `Phys_Init` verifies the engine main owner and rejects reentry. It acquires initialization ownership before real sound-alias lookup and publishes `physInited` after real task/system/broadphase, callback and surface initialization completes. Canonical repeated calls still refresh sound aliases/timestamps; Windows publication order is unchanged.
- Native manager initialization checks non-null, positive, non-wrapping storage and sufficient space for the actual aligned manager plus payload. It constructs the real manager before publishing its globals; duplicate ownership terminates. Existing assertions remain. Assertions declining a debug break cannot permit invalid storage publication.
- Native permanent allocation rejects missing manager/invalid alignment, and genuine exhaustion uses the existing allocation-safe terminal OOM service with the requested bytes. The existing zero-size allocation contract is retained. Successful cursor/allocator behavior is unchanged.
- SAP allocation uses its actual native `alignof` instead of the original four-byte Windows alignment. Broadphase memory's original eight-byte alignment is already correct. Canonical constructors initialize the actual algorithms; no substitute graph was introduced.

Evidence:

- `build-android-audit/native_registry_closure/physics-layout-honor.log`: actual device sizeof/alignof output.
- `phys_main-native.log`, `phys_mem_new-native.log`, `phys_broad_phase-native.log` in that directory: all real NDK translation units compiled, exit0.
- `build-android-audit/takeover_physics_init_windows1/{x86,x64}/build-result.json`: all three translation units compile in normal Windows configurations. Final adjustments affect only the Android branch, so those passed Windows bodies remain unchanged.
- `physics-arena-init-honor.log`: independent UBSan fixture using original manager/permanent allocator plus actual OOM owner passes null/zero/small/wrapping spans, deliberately unaligned dirty storage, completed manager map/span, actual topology alignment including a forced four-byte cursor offset, unchanged arena budget, duplicate ownership SIGABRT, and true exhaustion exit3. `_tlAssert` is an observation seam that deliberately declines a break to test the additional terminal checks; no production assertion was replaced. Headless/Monkey queries select the fixture's real terminal exit policy. No physics collision algorithm is faked or claimed tested.

Reproduce from BlackOpsARM:

```powershell
& tools/port/build_native_physics_arena_fixture.ps1
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 push build-android-audit/native_registry_closure/physics-arena-init-tests /data/local/tmp/blackopsarm-physics-arena-init-tests
& D:/Android/Sdk/platform-tools/adb.exe -s AKWJ9X5116G04618 shell 'chmod 755 /data/local/tmp/blackopsarm-physics-arena-init-tests && /data/local/tmp/blackopsarm-physics-arena-init-tests'
```

Frozen production files SHA256:

- phys_main.cpp: CBDCAF591DB2883CEEB0C3D38D931E22DF2F44D9CA473A34FEAD015ADD57F9E0
- phys_mem_new.cpp: 2D9B8356DAA7D0C1E0B15133DF56A7151D29CF59D9F6FC0BC31111E4471058A6
- phys_broad_phase.cpp: 1DE98705E069A13E74C995DA9F32360E21D713E2673CF024206CF74AF3CF3401

Integration: existing production translation units only; no new CMake source, header or flag. The previously selected real `oom_error.cpp` satisfies permanent allocation failures. Full production execution through physics remains the next proof; these checks do not certify Com_Init completion, Com_Frame, physics gameplay, rendered behavior or whole-engine recovery.
