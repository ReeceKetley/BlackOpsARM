# Native quit publication and lost-device terminal shutdown

2026-10-13. Actual native quit boundary implemented and fixture-tested on HONOR.
Full Com_Init/Com_Frame and renderer device lifecycle remain incomplete.

quit_services owns an atomic, process-lifetime quit request. Setting it publishes
with release; readers acquire it. Repeated requests remain latched. Android
Sys_SetWin32QuitEvent publishes the same request before signaling an initialized
render-owner event; before that event exists the durable native request remains
available. Windows implementation unchanged. No false event/device readiness.

Sys_QuitWithLostDevice preserves the working Windows reference's emergency
terminal exit from a renderer shutdown blocked by a lost device. It writes a
bounded local diagnostic directly to stderr, handling EINTR/partial writes,
then _Exit(0), avoiding engine/stdio output locks and atexit/detach handlers.
This is the existing deliberate terminal fallback, not successful renderer
recovery or ordinary clean shutdown. Windows console-file emergency append is
not implemented by this native service; native stderr captures the diagnostic.

The original backend close-during-simulated-device-loss test now requests actual
native quit and signals the existing quit event instead of posting WM_CLOSE on
Android. Windows diagnostic unchanged. Real SDL window focus/close dispatch and
Android application lifecycle are still pending; no fake WindowProc or ignored
events supplied.

Evidence:

- quit-build1 compiles production service plus focused test for Android28 ARM64
  with UBSan minimal runtime and static C++/sanitizer runtime.
- quit-run1 on HONOR passes initially-clear state, actual cross-thread request
  publication, repeated latch, child creation and child exit0 without executing
  its registered atexit handler. The terminal test exits only the child process.
  Does not execute original renderer device-loss loop or engine shutdown.
- full-engine-link172 compiles real service/thread wrapper; native backend still
  failed on WM_CLOSE and focus Windows types. Close diagnostic adapted;173 now
  stops on real Windows focus/display assumptions before linking.
- takeover_native_quit_windows1: two whole normal qcommon/threads and rb_backend
  sources pass Windows x86/x64, commands/manifests/build-result.json retained.

Only new test executable transferred. Assets and pinned DXVK unchanged. SDL
native HWND is associated with the SDL window in the tested native WSI; current
engine APK DevelopmentActivity is still a packaging/JNI shell and establishes
no SDL owner. Next integration must create/use the correct owner, pump events on
that owner and route actual focus/resize/close/input into engine lifecycle.
Then adapt remaining backend/entry dependencies and run genuine startup.
