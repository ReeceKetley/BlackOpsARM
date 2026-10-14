# Native renderer work publication and original event owners

Original qcommon/threads.cpp renderer handoff functions are selected beside the
actual native thread/event lifecycle. Sys_RendererSleep/Ready/WakeRenderer use
native-width atomic pointer exchange/load/store with acquire/release ordering.
Original completion, worker-command wake, backend wait, renderer stop/start use
real existing event providers. Windows bodies remain unchanged.

The native CPU-query selection still avoids duplicate Win32 thread creation/TLS
owners; the handoff selection restores genuine canonical algorithms, not fake
readiness. Native Sys_WaitForSingleObjectTimeout keeps its existing real CV/event
owner. The renderer completion event signals work completion, not GPU/device
initialization.

Production archive nm confirms all eight owners. Five normal Windows whole TUs
(script save/readwrite, engine gamepad/client gamepad, threads.cpp) bothABIs pass.
Reproducible bo1_native_render_handoff_tests target builds with UBSan. HONOR
AKWJ9X5116G04618 executes actual original handoff/event methods for512 producer/
consumer cycles, verifying payload bytes, pointer consumption, empty state,
completion/wakeup, auto-reset/backend timeout and renderer stop/start events.
Events retire only after the consumer joins. Test callback is finite and no GPU,
backend registration, command interpretation or actual renderer-loop proof is
claimed.

Evidence build-android-audit/native_registry_closure/render-handoff-build2.log,
render-handoff-push1.log, render-handoff-run1.log,
script-gamepad-handoff-windows1.log and current production libbo1_native_entry_state.a.

Caller must maintain the original single-producer/one-outstanding-work lifetime
contract and wait for actual completion before reusing payload storage. The
single-slot API is not a multi-producer queue. Full engine still cannot link and
real Com_Init/Com_Frame/server/render lifecycle remain incomplete.
