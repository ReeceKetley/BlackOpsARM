# Native resource services source closure - 2026-10-13

Com_Init remains unfinished. More original resource/recovery modules are now
selected in the native libraries; the full executable still does not link.
This report distinguishes implemented source/platform behavior from runtime
verification and retains the validated BlackOps64 reference policy.

## Production integration

Full original r_dvars, static-model cache, water/workercmds/FFT, DObj management/
dobj and collision loader sources are selected. The prior selected r_dvars and
cm_load objects are removed from entry_state to retain one canonical owner.
Original DObjInit replaces its inherited unavailable endpoint only in full builds.
Full dvar registration is now compiled; this does not prove it ran through startup.

Original resource queued/flushed wrappers, graphics-registration query/set/reset,
device-status query and Sys_WaitRenderer bodies are moved outside the selected
thread gate with their canonical event storage. Native backend spawning creates
the original 17 event modes/states, starts a real thread, installs role1/TLS and
publishes actual thread readiness. Graphics registration remains unsignaled.
The original initial device-status event is synchronization state, not proof that
an Android graphics device exists. Native OS scheduling replaces Windows priority/
ideal-CPU hints; exact hint parity is not claimed. Retirement wait/join accepts
real callback completion, with process-owned events and no restart/shutdown claim.

Renderer logging uses actual writable-user-root files, engine CRLF text services,
localtime_r/asctime_r and real flush/close. Canonical R_UnloadWorld/FatalInitError/
ErrorDescription support is selected. Android reports actual HRESULT bits and
terminal renderer errors via stderr/abort, rather than a Windows dialog. No
successful device creation, resource processing or error recovery was fabricated.

Water prefetch uses actual ARM compiler prefetch; unsigned status/counter operations
use real fixed32 full-barrier atomics. The Android spelling adapter now supplies
unsigned32 Interlocked overloads with wrapping bit semantics; it does not reinterpret
Android long64 storage as a Windows LONG. Missing cstring includes and signed32
invalid enum constants/forward declarations are corrected without layout weakening.
The existing DemonWare timing header uses native steady-clock nanoseconds and
actual sleep on Android; Windows QPC/Sleep paths remain unchanged. This timing
addition has not received focused runtime verification yet.

## Verification and pending execution

unsigned-atomic-build1/run1 pass eight HONOR UBSan checks: high status bits,
modular carry, failed/successful CAS, exchange, increment/decrement wrap and eight
real writers retaining all 80000 updates. This tests the new primitive overloads;
it does not certify every future FX/water counter access or scheduling pattern.

Nine whole normal Windows TUs pass both x86/x64 in
takeover_resource_services_windows1 (threads, init/dvars/logging, water/commands,
collision and DObj sources). Native expanded libraries compile; the current engine
link35 fails on 65 symbols after wider original code exposes additional animation,
model, resource, input, sound, UI and recovery dependencies. Symbol totals are not
a measure of percent complete; wider integration can reveal more dependencies.

bo1_native_render_services_tests compiles but has NOT run: its second build still
fails to link through actual localization/resource/recovery dependencies. It
contains finite test-owned backend callbacks and real logger/fatal checks; these
are not a replacement renderer or evidence of GPU registration. The first build's
missing string include is corrected and preserved. Full FS startup, enumeration/
localization and standalone pool relinks remain pending the genuine dependency
closure. Their historical passing results do not prove today's expanded graph runs.

All logs are under build-android-audit/native_registry_closure. Resource syntax
logs resource-closure-0..7-compile1 and resource-next-0..5-compile1 retain discovered
CRT/include/atomic/type blockers. full-engine-link31..35 preserve expanded link
failures, including duplicate inherited DObjInit corrected through source selection.
No placeholder was added to make the remaining graph link.

## Next work

Continue actual model/animation, effect/UI and render/input dependencies from
full-engine-link35. r_rendercmds still exposes Windows long counters that must be
made explicitly fixed32 through their canonical declarations and all users;
never fix them by adding a 64-bit Interlocked overload. Native sound/cinematic
backends also remain real dependencies. Preserve renderer-owned event initialization
and do not use test callbacks to signal genuine device/graphics readiness.

Relink and run the pending original FS initializer and extended enum/localization/
render tests once their actual dependencies are implemented, then resume full
Com_Init. Assets remain staged and reused; only the new atomic executable was
transferred. No commit/push, protected checkout edits or retail asset modification.
