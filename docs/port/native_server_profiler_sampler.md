# Original game profiler and native sampling integration

2026-10-13. Actual Com_Init remains incomplete; full game source now compiles ARM64.

Android server sampling uses a real CLOCK_MONOTONIC POSIX timer with 1 ms interval
and SIGEV_THREAD_ID delivery to its owning kernel thread. An exclusively installed
real-time signal handler captures the kernel ucontext ARM64 PC. Existing Windows
suspend/context/sampler behavior remains in the Windows branch; Android supplies
actual sampling rather than empty callbacks or fabricated instruction addresses.

The handler accesses only lock-free atomic sample fields and published category/
script-position values. Signal delivery is serialized on the owner thread. It
never traverses mutable VM frames from another thread. Original SvProfScope
category transitions publish their real values. Fourteen existing server VM frame
position assignments publish the position after the original assignment; client
instance writes are ignored. Script positions retain the original call/builtin
frame reporting granularity, not a new per-opcode profiler. Native profiler enable
and owning-thread globals are atomic so non-owner scope checks are safe.

End blocks its signal, disables sampling, deletes the timer, consumes pending
delivery, copies samples and restores the original mask before original report
aggregation. TLS destruction performs the same cleanup for a still-live timer.
Native aggregation is on the owner thread, preserving original hash-table policies
and address-range validation. Captured PCs retain uintptr_t width; only bucket
selection uses low32. Mode1 captures script-category samples; mode2 captures all
categories. Buffer/copy loss and timer overruns are counted and reported. Errors,
conflicting signal ownership, blocked delivery and invalid lifecycle fail explicitly.
Timer storage is per thread and bounded at4096 samples; no endless native sampler
thread or fake Windows suspension API was introduced.

HONOR UBSan server-sampler-run1 passes: 60 actual timer samples over two category
intervals, real PCs above4GiB, exact published script address, 25 script-only
samples, repeated restart, bounded-copy loss, live timer cleanup at thread exit
and a fresh thread's sampling/cleanup. Fixture captures production sampler source;
actual game category/VM integration and report execution are not yet runtime-tested.
No signal collision/error injection or4096-slot saturation test is claimed.
Only the small test executable was transferred; retail assets were reused.

Whole normal Windows game and script VM TUs pass x86/x64 compilation in
takeover_server_sampler_windows1. Final native atomic-global declarations are
Android-only; their Windows branches remain the declarations just compiled.

Full-engine-link109 confirms all selected ARM sources compile and reaches six
duplicate inherited diagnostics. FULL-only CL_Init, CL_InitDedicated,
CL_InitOnceForAllClients, CL_InitRenderer, CL_StartHunkUsers and
CL_ForwardCommandToServer diagnostic endpoints are now excluded because actual
original full cl_main_mp owns them. OFF diagnostic ownership remains as before.
Full-engine-link110 now reaches291 unresolved symbols across cinematic/audio,
client/UI/message/memory, renderer, input/network and remaining game owners.
No new placeholders were added. Final111 rebuild verifies native atomic flags.

Next select actual missing source owners (cl_main, sv_msg_write, ragdoll and
remaining client/UI/memory services) and adapt their true platform dependencies.
Actual sound capability/device and cinematic IO still need native owners.
Com_Frame is still an inherited unsupported endpoint; production startup and
frame completion must be proven by their real execution, not these source/probe gates.
