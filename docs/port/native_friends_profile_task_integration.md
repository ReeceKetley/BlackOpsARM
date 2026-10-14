# Canonical friends, profile and task owners

2026-10-13. Full production selects original live_friends_pc.cpp, live_clans.cpp
and win_tasks.cpp. The historical SocketRouter-only selector is retained only
for FULL_DATABASE=OFF: full production now compiles original live_win_common.cpp,
including actual player-card and UI-popup behavior. Existing BO1_LIVE exclusions
accepted in the working Windows reference remain unchanged.

Native player-card comparison and decimal formatting preserve the entire 64-bit
Steam identifier. Native friend enumeration uses the requested SDK friend flags,
validates output capacity and sorted table indices through real assertions, and
compares the actual formatted identifier during name/identifier lookup. Windows
branches retain their original behavior. No Steam client, friends or UI readiness
is fabricated; initialized SDK/UI execution remains unverified.

Canonical task clear writes the actual native boolean rather than aliasing it
through a four-byte integer. Original reference assignments use fixed32 native
atomic increment/decrement; destruction is based on the decrement result rather
than a second racing counter read. Windows interlocked paths remain intact. This
does not introduce a new reference destructor or change the original accepted
BO1_LIVE-disabled clear policy.

The focused HONOR UBSan fixture links actual win_tasks.cpp and instantiates the
canonical bdReference template. Three workers perform concurrent reference
acquisition/copy/retirement under a retained seed; final retirement invokes the
real object's destructor once. Actual 32-slot clear, allocation, progress and
retirement pass. Assertions in this fixture terminate immediately; it does not
implement a fake online task backend or claim DemonWare service initialization.

Reproduction: tools/port/build_native_task_reference_fixture.ps1; evidence
build-android-audit/native_registry_closure/task-reference-build1.log and
task-reference-honor-run1.log (exit0). Only the fixture executable was transferred.

Final nine whole normal Windows translation units pass x86 and x64 in
build-android-audit/takeover_faudio_friends_tasks_windows2 (original four sound
owners, friends/clans/live common/tasks and corrected gamepad owner). Earlier six
whole system/platform/friends/task units pass both ABIs in
takeover_system_friends_tasks_windows1; its manifests describe that snapshot.

Full-engine-link389 compiles these owners and the integrated real system, Steam,
Monkey, assertion and FAudio batches, then fails at 176 genuine undefined symbols.
The full executable has never linked or executed Com_Init/Com_Frame. Symbol count
is linker closure evidence, not a measure of startup completion or remaining effort.
