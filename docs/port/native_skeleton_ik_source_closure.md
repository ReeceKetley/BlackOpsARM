# Skeleton and IK source integration

2026-10-13. Full Com_Init remains incomplete.

Full native library selects original DObj skeleton and animation calculation,
IK/import/math/process, rope and client-game sources. Full builds remove the
earlier IK-only entry selection so state and implementations have one owner.
Real client skeleton creation, IK part masks and box distance now resolve from
canonical original source. No substitute initializer or readiness endpoint.

Portability changes: native IK debug formatting uses vsnprintf, va_end is paired,
and formatted text is passed as data to Com_Printf. Native PB token comparison
uses original I_stricmp. Matrix setter/interpolation/debug-origin and the physics
axis conversion now address actual rows, preserving values while removing the
identified flattened array-subobject accesses. Interpolation row copies use
memcpy rather than potentially unaligned/aliased 64-bit loads. Other decompiled
math accesses still require audit; this is not a global UB-free claim.

## Verification

- All selected sources compile ARM64; full-engine-link72 still fails on actual
  cinematic/sound, physics object/GJK, glass, client state, network/input/recovery
  dependencies. Logs68..72 preserve this source expansion and compiler/link errors.
- Eight complete Windows TUs pass x86/x64 in takeover_skeleton_ik_windows1.
  Final math/import/physics-header changes pass three whole TUs on both ABIs in
  takeover_skeleton_ik_windows3. Intermediate windows2 catches a mechanical edit
  accidentally changing matrix parameter dimensions; signatures restored before
  execution. Its failed evidence is retained; native link71 is superseded by72.
- Actual original ikMatrixSet44 executes on HONOR with UBSan in the focused
  native_ik_matrix_tests executable. Nontrivial axis rotation, scale, translation,
  homogeneous row and adjacent bounds match the expected result. Evidence:
  native_registry_closure/ik-matrix-build1.log and ik-matrix-run1.log.
  Whole original ik_math.cpp is compiled with function sections; the test invokes
  the setter only. This is not full IK, animation or startup execution.
- Only this 82 KB test binary transferred; game assets reused, DXVK/Vulkan unchanged.

Continue actual Com_Init closure from link72: physics/GJK, glass, cinematic/sound,
client state and platform event/input/network/recovery still require their real
owners. Do not repeat completed isolated probes absent a new relevant change.
