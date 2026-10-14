# Original client and game source integration

2026-10-13. Actual Com_Init remains incomplete. Full target currently fails
compilation in the original full game source's added Windows server profiler.

The full database target now selects original cl_main_mp, g_main_mp, cg_weapons,
ragdoll_controller, destructible, xdoll and glass_shard. The reduced client-state
and map-name entry selections are removed for FULL builds to preserve one owner.
Six of these sources compile on ARM64; full g_main_mp still does not. No new
unavailable endpoints, fake initialization or assertion bypasses were added.

Native destructible prefetch uses the compiler's read/locality1 primitive;
Windows retains its original intrinsic. Glass local timer variables now use
the canonical BO1_TL_COUNTER type already returned by tlPcGetTick. CP1252 source
encoding is preserved. Explicit signed32 enum sentinels are -1, retaining their
0xffffffff bit pattern. Win32 GUI structures and window procedures are declared
only for the Windows backend; public console functions remain declared.

The original client environment command now uses a native copying Putenv
service. It parses NAME=VALUE, delegates to setenv with replacement, uses unsetenv
for NAME=, validates missing/empty names, propagates real OS errors and frees
temporary name storage. POSIX putenv would retain the client's stack buffer.
HONOR UBSan environment-run2 passes copied lifetime (caller buffer overwritten),
replacement, removal, absent removal and invalid input with existing-value
preservation. Forced allocation failure and concurrent environment mutation were
not tested. Initial run1 could not load its dynamic sanitizer runtime; build2
links sanitizer runtime statically and passed. Logs remain preserved. Only the
test executable was transferred; game assets were reused.

Seven full normal Windows translation units pass x86 and x64 in
build-android-audit/takeover_client_game_windows1. After additional game changes,
that full TU passes both again in takeover_game_profiler_windows1. These are
compile comparisons, not gameplay or profiler execution tests.

Game profiling now uses canonical BO1_ProfileTicks/ThreadId (Windows TSC/TID,
Android monotonic nanoseconds/TID) and native QPC types/functions. Its Windows
DbgHelp include is platform-conditional; all profiler bodies remain selected.
Latest full-engine-link106 preserves actual unresolved compiler dependencies:
server thread suspension/context sampling, sampler creation/delay, process virtual
memory reporting and Windows symbol/DLL lookup. The native implementation must
preserve native-width instruction addresses and implement real synchronization
and lifecycle, rather than fabricate samples or silently disable profiling.
Logs103..106 preserve the corrections and remaining failures.

After this compile closure, audit duplicate FULL diagnostic endpoints against
the newly selected real CL_Init/CL_InitDedicated owners, resolve remaining link
dependencies, then execute actual Com_Init. No new full-entry runtime result is
claimed. Audio, cinematic, graphics and remaining engine owners are still pending.
