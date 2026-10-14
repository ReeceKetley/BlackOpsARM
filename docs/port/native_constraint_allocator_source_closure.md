# Constraint and physics allocator integration

2026-10-13. Full Com_Init remains incomplete.

Selected original physics_system_internal/phys_mem_new/phys_assert,
tl/physics/rbc_def_generic, bg_animation and scr_const sources. These provide real
constraint setters, physics-system storage, bgame TLS/state and script constant
storage from their canonical owners. Storage selection is not initialization;
the original game/script startup calls remain necessary.

Native physics linear arena allocation uses real native-width atomic cursor
loads/CAS, bounds/overflow checks and valid alignment admission. Slot ownership
CAS uses the existing pointer-width native primitive; allocated-slot counters
update their declared four-byte fields instead of casting them to LP64 long.
The legacy temporary-pointer exchange used as a barrier maps to a real native
memory fence. Constructor clears the actual pointer-array size rather than the
Win32 256-byte literal. Windows branches retain their original behavior.

Original animation parse-error formatting uses native vsnprintf and pairs
va_end before the genuine engine error path. No fake allocator, ready state,
constraint operation or script initialization introduced.

## Evidence and limits

Logs77 preserve native API/long-width compilation blockers;78/79 compile
expanded sources and fail at remaining real linker dependencies. Six complete
Windows TUs pass x86/x64 in takeover_constraint_state_windows1. Later pointer-map
clear/alignment/bounds edits are Android-only, Windows body unchanged.

Actual original phys_memory_manager constructor/allocate execute on HONOR with
UBSan. Test verifies all native pointer-map entries clear, misaligned arena/
arbitrary-size allocation,512 concurrent nonoverlapping owners, invalid alignment
and oversize requests retaining cursor, exact remaining capacity and exhaustion.
Evidence: native_registry_closure/physics-linear-build1.log and
physics-linear-run1.log; tools/port/tests/native_physics_linear_allocator_tests.cpp.
Only its small binary transferred; game assets untouched.

This is the linear arena, not full tagged slot-pool allocation/free/ownership,
physics-system initialization, constraint simulation, game/script startup or
Com_Init. Those paths remain unverified. Retained original source compilation
does not establish initialization ordering or runtime numerical correctness.

Continue from full-engine-link79: actual sound bank/patch, cinematic, UI/client/
ragdoll/destructible, remaining physics helpers, input/network/recovery owners.
Prior source/link failures and passing tests remain preserved with their scope.
