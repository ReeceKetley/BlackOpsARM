# Genuine native Sys_Init core

`win32/win_main.cpp` restores the canonical `Sys_Init`, `Sys_In_Restart_f`,
`Sys_Net_Restart_f`, `Sys_Mjpeg`, and `Sys_MjpegClose` bodies under the production
`BO1_NATIVE_SYSTEM_CORE` selection. Actual command callbacks register `in_restart`,
`net_restart`, `movie_start`, `movie_stop`, `net_listen`, and `net_connect`.
Movie/network handlers retain their genuine dependencies rather than successful
placeholder callbacks. Windows-only UI/headless-harness controls are not advertised
on Android while their Windows consumers remain excluded. Windows branches retain
their original behavior. Existing `Sys_LoadingKeepAlive` and tagged-payload retirement
are unchanged by this batch.

Native service files:

- `platform/android/system_initialization.cpp` validates actual kernel identity
  with `uname` and actual `CLOCK_MONOTONIC` resolution with `clock_getres`.
  Query failures use the genuine fatal error path. It does not emulate a Windows
  version or synthesize timer availability. The canonical system-information log
  uses `%Lf` for Android's native long doubles and identifies OS frequency and
  reference configuration estimates honestly.
- `platform/android/input_initialization.cpp` registers canonical `in_mouse`,
  polls the real console producer already installed by native main (validating
  its actual owner), and invokes canonical `GPad_InitAll`. Headless mode opens no
  fabricated physical device. Rendered initialization fails explicitly until
  actual keyboard/mouse/touch translation is bound to the SDL event/Java owner.
  Shutdown invokes actual `GPad_ShutdownAll`; restart preserves the separately
  owned process console stream. Input identity is bound once to its actual pthread
  and remains immutable for the process. Initialized state uses acquire/release
  atomics; lifecycle callbacks require the original owner.
- Native platform descriptors forward clock/quit/print, FS read/free/write, and
  input lifecycle calls to actual existing services. Bootstrap requires an
  initialized engine filesystem and publishes only those three implemented
  interfaces. Audio, rendering, online services, and video stay null.

The original `platform/platform.cpp` registry is reused. Its completeness predicate
remains false for that partial registration and logs bootstrap incomplete.
Native clock/print helpers can use their individually registered real service;
early fallbacks call native clock/output owners, not Win32 entry points. The duplicate
generic `Platform_InputFrame` definition is excluded on Android: the actual
`console_input.cpp` frame service continues to own it. No independent registry,
dummy descriptor, fake readiness, or event consumer is introduced.

## Build scope and remaining dependencies

Four actual ARM64/API28 UBSan whole TUs compile:
`win_main.cpp`, native system initialization, native input initialization, and
the original platform registry. Evidence under
`build-android-audit/native_system_core` includes `win_main-build2.log`,
`system_initialization-build2.log`, `input_initialization-build2.log`, and
`platform-build1.log`. These use the existing native headless/system-core production
selection, not retired startup/string-pool test gates. Root owns CMake selection,
normal Windows comparisons, diagnostic endpoint replacement and full linking.

Required selected paths are the two new native `.cpp` files and original
`platform/platform.cpp`. Genuine dependencies include existing filesystem,
console input/output, clock, quit, canonical GPad/controller service, network
restart/listen/connect and MJPEG creation/close plus actual renderer dimensions
and clip Dvars. Original `win_input.cpp` remains omitted until coordinated native
rendered-input ownership is available; it must not duplicate these native lifecycle
functions. Audio/render/online/video interfaces remain explicitly unbound.

All fatal prerequisites call actual `Com_Error(ERR_FATAL)` and then `abort` if
that fatal callback unexpectedly returns, preventing descriptor/input publication
after failed prerequisites. No new lock is held across the callback.

## Combined focused execution

`native_system_core_tests.cpp` executes actual canonical Sys_Init with actual
Dvar/Cmd/string pools/native machine information/GPad initialization and the real
installed console producer/canonical event queue. The scoped filesystem provider
owns a genuine initialized temporary directory and actual file/buffer IO; it is
not the production engine filesystem. Unexecuted network/movie/expression and
other callbacks terminate explicitly if reached. All registered Dvars are actual
canonical objects, not manufactured fixture values. The original platform registry
and new native adapters are executed directly.

HONOR UBSan run2 exposed a genuine headless restart failure: canonical GPad_InitAll
opens no physical controller service, but ShutdownAll required that absent SDL
owner. Its narrow native/headless counterpart now retires the actual logical pad,
generation, sample, and delayed-input state without touching an unopened physical
backend. Rendered shutdown remains the original owner/SDL shutdown path. The
failure evidence remains `native_system_core/honor-run2.log`.

After that correction the actual combined fixture passes exit0,
`SYSTEM_CORE PASS checks=30`, in `native_system_core/honor-run4.log`. It proves six
canonical core command callback bindings; actual mouse/gamepad Dvar state; genuine
pipe input queued during Init and after actual in_restart; only three descriptors
published with four null and original Ready=false; actual isolated FS adapter IO;
and expected abort after intentionally returning fatal callbacks for missing FS,
failed OS query, wrong input owner, and unsupported rendered input.

`tools/port/tests/build-native-system-core.ps1` reconstructs a selected object
archive from preserved production canonical dependency objects and builds current
core/input/registry/GPad sources. Only the standalone canonical FCS fixture uses
its historical narrow source selector; full production remains ungated. Selected
object SHA256 records are emitted by the recipe. Reuse the already staged SDL2
library with `LD_LIBRARY_PATH=/data/local/tmp/blackopsarm-native-engine-entry47-1`.
Push requires restoring executable permission. Failed linker/runtime logs are
retained, including the observed restart failure.

This result does not establish actual production filesystem initialization,
network/movie callback execution, physical/rendered input, full recovery,
Com_Init return, or Com_Frame execution. Existing scoped tests retain their
original meanings. Only the new fixture binary was transferred; no assets,
protected checkouts, ledger database, commits or STATE were changed here.
