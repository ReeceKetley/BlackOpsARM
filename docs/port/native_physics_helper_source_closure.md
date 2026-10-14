# Physics helper and vehicle source integration

2026-10-13. Full Com_Init remains incomplete.

Selected original physics util/render/broad-phase/GJK collision detection/rigid
body, vehicle/contact constraint definitions, GJK separation direction,
transient allocator and nitrous vehicle sources. Ten actual owners now compile;
matrix/cross/conversion functions were found in the vehicle source and reused,
not reconstructed independently. Broad-phase destruction/list ownership and
vehicle/contact operations likewise remain original engine implementations.

Transient allocator excludes the Windows intrinsics header on Android, using
the existing real native atomic compatibility primitive. Its multithreaded
cursor is loaded atomically; alignment/negative-size and integer overflow bounds
are checked before pointer advancement. Windows branch retains original behavior.
No allocation or physics state is fabricated and no convergence limit altered.

## Evidence

Native source compilation succeeds; full-engine-link99 still fails at actual
remaining audio/cinematic/client/game/script/renderer and physics dependencies.
Logs96..99 preserve this expansion and missing intrin.h compilation failure,
corrected by using the existing native intrinsic implementations.

Whole normal Windows x86/x64 comparison passes for seven TUs in
takeover_physics_helpers_windows1 and three in physics_vehicle_windows1.
No tablet probe or transfer in this step. Transient allocation, matrix/vehicle
and broad-phase/GJK runtime behavior remains unverified. Earlier linear arena
test does not certify this allocator's complete block/slot lifecycle or these
new physics operations. Preserve that evidence with its original scope.

Continue actual Com_Init closure from99: native audio capability/device,
cinematic, client/game/script state, ragdoll/destructible, remaining physics
geometry/task/solver owners, input/network and recovery. Keep original widths,
resource ownership and assertions; do not treat compiled sources as initialization.
