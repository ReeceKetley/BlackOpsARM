# SDL event owner and renderer lifecycle boundary

2026-10-13. Full rb_backend now compiles ARM in link176; complete executable
still fails linking real dependencies. No Com_Init/Com_Frame/rendering result.

window_lifecycle implements SDL event subsystem ownership with one explicit
thread, mandatory engine event consumer, window attachment/detachment and retirement.
Actual SDL_PollEvent runs only on that owner; non-owner callers read mutex-protected
snapshots and submit cursor requests for the owner. Every event is forwarded to
the consumer outside the snapshot lock, including key/touch/controller and app/
window events. No input event silently discarded. Actual quit/close/termination
events publish native quit state. Window flags/size come from SDL; window/app
focus and foreground events update lifecycle state. Missing/unattached window
queries fail without publishing fabricated readiness.

Native backend normal and lost-device message loops now use the lifecycle boundary
instead of Win32 message pumping. Backend checkpoints require the real installed
SDL window/owner. If render and SDL owners differ, only the SDL owner may pump;
renderer consumes snapshots, so engine main/input integration must continue
pumping events while render work proceeds. Neither owner nor window has yet been
installed by production engine startup: this remains required work.

Native focus diagnostic queues logical SDL window focus events and logs actual
delivered focus/foreground/flags. It does not change the Android OS display mode
or claim real activity focus loss. Return phase waits for owner-delivered focused
state before reporting return. Actual Windows test behavior unchanged. Original
D3D9 Present, device-loss/reset/resource/recovery logic remains retained; no fake
recovery/device success. Correct setjmp header resolves the later native declaration
gate. Native-only render-thread scratch pointer now has internal linkage, avoiding
its unrelated collision with water simulation global data; external users were
not found. Windows scratch linkage unchanged.

Full target imports existing protected DXVK-Native SDL2 Android runtime and matching
headers. Runtime reports2.30.11 and SHA256 is
11c28146f62f5bb13cee863c1203d574324ef83931f1c668a3a181b470235e11.
Native WSI explicitly casts HWND to/from SDL_Window*, matching the backend adapter.
Protected sources/binaries unchanged; no Vulkan requirement change.

Evidence:

- sdl-lifecycle-build1 failed because SDL remapped test main; corrected standalone
  test uses SDL_MAIN_HANDLED/SDL_SetMainReady; build2/3 pass production service plus
  fixture with ARM64 API28 UBSan minimal runtime.
- HONOR sdl-lifecycle-run1/2 use actual SDL event queue with injected key/app/quit
  events. Run2 verifies owner-only pumping, non-owner rejection, callback owner,
  exact-once delivery, real native quit latch, absent-window/focus rejection and
  event subsystem retirement. No video/window creation, positive focus delivery,
  resize/cursor behavior, real Android Activity callbacks or GPU tested.
- full-engine-link174 reaches missing jmp_buf declaration; corrected175 compiles
  backend and reaches duplicate data. Link176 corrects scope and reaches genuine
  audio/cinematic/LAN/demo/game/client/renderer/platform linker dependencies.
- takeover_sdl_backend_windows1 passes whole normal backend on x86/x64. Subsequent
  standard setjmp header/native scratch and native-only focus-return edits leave
  original Windows behavior; no new Windows runtime result claimed.
- Final SDL_APP_TERMINATING case and wait-for-delivered-focus guard added after
  run2/link176; these two changes are pending next compilation/test. Preserve
  evidence scope rather than calling these exact final edits runtime-tested.

Transferred only missing matching libSDL2.so once and changed test binaries.
Retail assets reused. Next: bind actual engine SDL video owner/window and translate
mandatory input consumer into engine event queue, then validate real lifecycle.
Continue remaining full link176 dependencies and pending worker-context probe.
