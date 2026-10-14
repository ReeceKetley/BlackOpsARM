# Native engine frame timing

Updated 2026-10-14. Measurement instrumentation is integrated into the actual engine sources; whole translation unit checks pass. Production tablet measurements with these edits are pending the root agent's next combined build/run.

## Timing interpretation

The validated BlackOps64 reference and Android worktree both register `sv_fps` with default 20 and compute `frameMsec = 1000 / sv_fps`. The normal simulation timestep is therefore deliberately 50 ms; this is not an Android-only headless throttling workaround. Display/rendering FPS is a separate rate. Changing the server tick to chase 30 rendered FPS would alter simulation behavior and is not part of this measurement change.

`Com_ModifyMsec` emits the dedicated hitch warning from elapsed **main-loop** milliseconds before clamping that elapsed value to 5000 ms. `SV_FrameInternal` consumes the resulting accumulated time in repeated server updates. At 50 ms per update, a clamped 5000 ms can schedule 100 updates in a single main `SV_Frame` call. The existing approximately 7.4-second warnings every 100 frame markers are therefore consistent with catch-up batches, rather than every individual server update taking 7.4 seconds. This is a source-supported interpretation awaiting measured batch durations; it does not identify CPU, scheduling or synchronization as the cause.

The complete process runtime includes initialization and asset loading. Dividing total completed simulation frames by time since process launch does not establish steady-state update throughput or `SV_Frame` execution cost.

## Actual instrumentation boundaries

All measurement bodies are Android-only. Windows engine algorithms, canonical simulation times, residual accounting, error paths, waits, ownership, completion ordering and assertions remain in place. This change makes no pacing or performance optimization.

- `owner=server`: the actual server owner reports `server_work`, covering ownership acquisition/pings, preframe, run frame, save queue, postframe and final event processing. Separate inclusive phases identify `server_pre`, `server_run`, `game_run` (actual `G_RunFrame`), `server_save` and `server_post`. `network_loop` and its actual network event wait are also measured. Work stops before diagnostic output and completion publication.
- `server_idle` measures the actual wait-start-server block, including its canonical optional worker assistance. CPU time distinguishes busy assistance from elapsed idle waiting. `wake_reset` measures `Sys_SleepServer`, which actually polls/reset the wake event with timeout zero; it is not the sleeping wait-start block.
- `owner=main`: `main_frame` measures the existing `Com_Frame_Try_Block_Function` body. `sv_frame_main` measures the actual main caller's `SV_Frame`, which may include a whole catch-up batch and synchronization; it must not be called the server thread's per-update execution cost.
- Main synchronization is split into inclusive `server_sync_main`, actual `completion_poll` calls, actual `assist_work`, the stale counted-handshake loop and its `yield` calls, plus network completion waits. Canonical `Com_SyncThreads` measures inclusive `com_sync`, `render_sync` and `worker_sync` boundaries.
- `pacing_sleep` measures actual `NET_Sleep(1)` calls in both canonical frame pacing branches. The outer entry loop does not introduce a separate sleep.

The header uses actual `CLOCK_MONOTONIC` elapsed time and `CLOCK_THREAD_CPUTIME_ID` CPU consumed by the **calling** OS thread. It preserves errno and marks invalid/unavailable clock samples explicitly instead of inventing timings. No cross-thread aggregation, allocations, fake services or synthetic function implementation is used. Helper functions exclude themselves from the existing compiler function tracer to limit diagnostic overhead; the engine's production instrumentation and UBSan remain enabled.

`BO1_NATIVE_TIMING` emits one bounded stderr line for the first three completed server updates and then each 100 updates. Main output is limited to the first three observed wake counts and crossings of each 100-wake group. Server index is completed level-frame count; main index is scheduled wake count, **not** an assertion of completion. Startup/map-loading idle is excluded from the first server work window; main aggregation begins after an initialized running server was observed at the start of the frame. A map command executed within that first main frame is deliberately excluded from the main work average.

Each phase contains `n` calls, `w_valid` and `c_valid` valid sample counts, `w` elapsed total milliseconds, `c` calling-thread CPU total milliseconds, and `w_max` / `c_max` maximum individual-call values. Missing clocks have a value of -1; partial clock failures are evident from valid counts below n. Average wall or CPU cost is total divided by its respective valid sample count. The monotonic `window_wall_ms` includes real gaps and other work between samples, while excluding the current timing report's own output. Nested inclusive phases overlap; **do not add their totals**. CPU subtraction from wall is not a pure synchronization measurement: it also includes descheduling, I/O and other non-CPU elapsed time.

`BO1_NATIVE_HITCH` accompanies the unchanged canonical hitch warning. It records raw engine msec, the unchanged dedicated clamp, previous main-frame monotonic/CPU duration, previous main `SV_Frame` elapsed duration and scheduled wakes in that main batch. First unmeasured or unavailable values are marked -1. This directly compares the hitch detector against real batch cost without altering its accounting.

Recoverable nonlocal engine errors can skip a live measurement scope; such interrupted work is not falsely reported as a completed sample. No timing output claims recovery success. The existing completed-frame markers remain unchanged.

## Verification

- Actual NDK30/API28 production compile commands replayed to isolated objects for `src/server_mp/sv_main_mp.cpp` and `src/qcommon/common.cpp`: both exit 0, O0/debug/function instrumentation/UBSan/no-recovery preserved.
- Normal Windows x86 and x64 whole translation units for both sources: exit 0 using the existing comparison script with `/permissive /DWIN32=1`. Protected BlackOps64 was only read.
- Read-only oracle checks confirm sv_fps=20, integer frame step, residual catch-up loop and dedicated 5000 ms clamp match the working Windows implementation.
- Existing evidence full428 is from **before** this timing change. It proves 1300 completed server/script frames and clean mapped quit, but does not supply the new monotonic/CPU timings.

Evidence: `build-android-audit/native_frame_timing1/` retains byte-identical source-before copies, scoped diffs, exact compile commands/logs, native results, Windows results and the final three-source SHA256 manifest. The first native compile helper attempt failed before invoking a compiler due to Python regex replacement interpreting a Windows path; replacing the output with a forward-slash path fixed that setup problem. No standalone framework or repeated broad test suite was added.

Next production run should preserve the unchanged staged assets and record these lines from the combined engine executable. Compare server_work/GameRun means and maxima, main batch wake count and duration, wall/CPU separation, polling/assistance, pacing and event waits. Optimize only after actual measurements identify a bottleneck.

## Actual full429 follow-up

The original broad-phase instrumentation has now executed on the tablet. Stable actual full429 windows prove approximately 13.526 completed updates/sec, 73.62ms elapsed/73.28ms CPU work per update, with postframe approximately 62.46ms and 84.8% of work. The recurring 7.4-second warning matches a 100-update main catch-up batch. See [Actual frame measurements and postframe split](native_postframe_timing.md) for the exact windows, evidence, added coarse archive/client/post measurements and controlled diagnostic-overhead comparison proposal. These statements supersede the initial pending-production boundary above only for the original broad phases; new phase results remain pending.
