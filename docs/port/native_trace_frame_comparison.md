# Same-executable startup trace comparison

Updated 2026-10-14. Both same-executable full430 runs are terminal with exit 0. Actual measurements identify the continuing startup trace scan as substantial post-init overhead: removing that diagnostic scan after genuine Com_Init enables sustained canonical 20 Hz headless simulation while preserving real archive work, restart and mapped quit.

## Baseline scope and map epochs

Actual evidence: `build-android-audit/native_registry_closure/full-engine430-honor-restart-baseline1.log` and its runner JSON. Command metadata explicitly sets `BO1_TRACE_POSTINIT=1`, uses the unique `full-engine430` executable and unchanged assets at `/sdcard/boARM`, and reports no asset transfer. The runner sent map at 40.085749s, actual `map_restart` at 190.071072s and autoquit at 300.071037s. Process exit0 at 309.814278s. Parent independently verified clean guard pid 8181.

The log proves two actual map epochs. First G_InitGame/SV_SpawnServer complete at level time 0/frame 0/script 0. Frame count reaches 300 and level time 15000. The canonical restart calls ShutdownGame(1), reinitializes the map, and both completion markers report **level time 15000/frame0/script0**. New frame/script counts reset, while server time and process wake/serving counters continue. New level frame300 reaches level time30000 and process serving 600. Treat `(epoch, level frame)` as the timing key; frame200 before restart and frame200 after restart are distinct windows. Resetting level frame count is not resetting the server's process counter or cumulative simulation time.

## Actual baseline phase results

The per-window values below use completed updates as the denominator. Initial first-three-update timings are retained in the JSON evidence but excluded from the stable-window table. Each frame100 aggregate contains 97 updates because first 3 were emitted separately. Server window spans exclude the map initialization/restart loading interval; main frames containing restart correctly include that work and must not be mistaken for steady simulation cost.

| Map epoch | Window ending at level frame | Updates | Updates / real second | Work / update | Postframe / update | Archive serialization / update | Archive commit / update |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| First map | 100 | 97 | 12.787 | 77.905 ms | 66.468 ms | 63.870 ms | 0.0198 ms |
| First map | 200 | 100 | 12.709 | 78.381 ms | 66.784 ms | 64.178 ms | 0.0207 ms |
| First map | 300 | 100 | 12.708 | 78.365 ms | 66.768 ms | 64.160 ms | 0.0222 ms |
| Restarted map | 100 | 97 | 12.794 | 77.855 ms | 66.428 ms | 63.835 ms | 0.0126 ms |
| Restarted map | 200 | 100 | 12.633 | 78.799 ms | 67.036 ms | 64.410 ms | 0.0143 ms |
| Restarted map | 300 | 100 | 12.732 | 78.233 ms | 66.735 ms | 64.140 ms | 0.0119 ms |

The actual first-map frame200 archive serialization is 64.178 ms elapsed /63.894 ms thread CPU per update. It accounts for approximately **96.1% of server postframe and 81.9% of actual server work**. Restarted-map values corroborate this. This now identifies genuine **archive serialization CPU work** as the dominant baseline inner owner, rather than a network or sleeping wait. It does not yet identify a particular serializer helper as the cause.

The real client build and transmit outer loops take only about 0.003–0.004ms per update in this no-connected-client run. No actual client snapshot build/write/transmit function samples appear. Canonical archive ring publication/copy costs about 0.012–0.022ms per update; it is small compared with serialization. The far-entity diagnostics cost about 2.5 ms per update. Debugger, timeouts, authentication, animation collection and event waits are also small in these measured windows. These costs are measured for this actual headless no-player state, not a connected rendered gameplay workload.

Both baseline and future comparison must retain archive serialization and its original wire/cache/ownership behavior. The baseline preserves O0, all current compiler hooks, UBSan/no recovery, assertions and canonical 50 ms simulation timestep. Full429's previous 13.526 updates/sec is useful earlier evidence but **is not the controlled baseline**: only these full430 values should be paired with the same430 executable's trace0 run.

## Controlled comparison boundary

Parent implemented an actual atomic native trace gate after genuine Com_Init return. `BO1_TRACE_POSTINIT=1` retains the startup-target scan;0 (the default) returns from the hook before scanning 14 entries. The compiler hooks themselves remain present. Engine functions, sanitizers, archive work, assertions, assets, executable and timestep remain the same between these two full430 runs. The actual startup coverage remains enabled until real initialization returns. This comparison isolates the ongoing startup diagnostic scan, not removal of engine implementation.

Both runs now pass real restart and mapped quit. The completed comparison below supports actual headless simulation and diagnostic-overhead conclusions; it does not establish rendered FPS or app readiness.

Analysis evidence: `build-android-audit/native_frame_timing3/full430-baseline-timing-analysis.json` retains complete parsed phases with map epochs, actual source line numbers, raw counts/valid sample counts, source SHA256 and runner metadata. No engine sources, compiler configuration, device state or protected checkout was edited by this follow-up analysis.

## Completed trace-scan-disabled run

Actual source `full-engine430-honor-restart-traceoff1.log` and runner JSON explicitly select `BO1_TRACE_POSTINIT=0`, the same unique executable and same staged assets. The runtime logs scan enabled=0 only after real Com_Init. Metadata reports exit 0 at 300.997788s, no asset transfer, map sent 40.083580s, restart 190.066900s, and autoquit 300.093818s. Clean guard pid 8435 is retained. Parent confirms full430 SHA256 **2D32496485634C67AF842DE1A94EDCA7F0E80E8DDC6F47D25204DAE4A08DC852** stayed unchanged for both runs.

Both runs have two actual G_InitGame/SV_SpawnServer completion pairs and one successful same-process map_restart. Baseline explicitly acknowledges serving 600 at restarted level frame 300/time 30000, then mapped quit with level.time30000. Traceoff explicitly acknowledges serving 3924 at restarted level frame 1600/time 196200, with script_time1600; the final quit snapshot advances level.time to196800. Thus at least 3924 completed process updates are directly acknowledged in traceoff. The later 196800/50=3936 simulation tick total is inferred from the final game-time snapshot and canonical 50 ms step; it is not a separate final WORK_COMPLETE marker. Both actual canonical shutdowns complete and exit 0. No UBSan runtime-error report was observed in either log.

The traceoff first epoch starts at cumulative server time 0, as the baseline does. Restart happens at time 116200 in traceoff versus 15000 in baseline because the faster simulation completed many more real updates before the same 190-second command. Level/script frame counters reset to 0, while cumulative server time and serving counters continue. This makes second-epoch absolute clocks different; comparisons at the same restarted map age are labelled accordingly rather than claiming identical absolute simulation state.

Every recorded phase in both parsed runs has n==w_valid==c_valid. No clock failures/unavailable timings are hidden in these results.

## Paired map-age measurements

These are actual mean elapsed costs per completed update, not per main-loop call. Both frame 200 traceoff windows are **transient catch-up**; their above 20 Hz throughput is not steady capacity. First-epoch frame 300 is the strongest matched steady pair: same map epoch, index 300 and absolute/relative simulation time 15000.

| Epoch / level frame | Baseline / off cumulative sim time | Relative map age | Work scan1 → scan0 | Post scan1 → scan0 | Archive scan1 → scan0 | Game scan1 → scan0 |
| --- | --- | ---: | --- | --- | --- | --- |
| First /200 | 10000 /10000 ms | 10000 ms | 78.381 →25.544 ms | 66.784 →18.467 ms | 64.178 →17.630 ms | 11.391 →6.932 ms |
| First /300 | 15000 /15000 ms | 15000 ms | 78.365 →35.619 ms | 66.768 →25.024 ms | 64.160 →23.799 ms | 11.391 →10.332 ms |
| Restart /200 | 25000 /126200 ms | 10000 ms | 78.799 →25.585 ms | 67.036 →17.962 ms | 64.410 →17.176 ms | 11.572 →7.472 ms |
| Restart /300 | 30000 /131200 ms | 15000 ms | 78.233 →36.557 ms | 66.735 →24.674 ms | 64.140 →23.552 ms | 11.292 →11.607 ms |

In the exact first-epoch frame 300 match, work improves from 78.365 ms elapsed /77.997 ms CPU to 35.619 ms /35.216 ms. Archive serialization improves from 64.160 ms elapsed /63.879 ms CPU to 23.799 ms /23.601 ms—about **2.70× less elapsed archive cost** and **2.20× less total elapsed work cost** for that window. Actual game simulation changes modestly from 11.391 to 10.332 ms. The restarted map-age300 pair shows similar archive reduction (~2.72×), but its absolute cumulative simulation clocks differ and its game cost is 11.292 versus 11.607 ms. No blanket fixed cost per update or matched absolute state is claimed for these restarted pairs.

The comparison preserves engine implementation, bytecode/assets, original archive/cache behavior, atomic/completion protocol, 50 ms step, O0, compiler hook calls and UBSan/assertions. The diagnostic gate removes ongoing target scanning and its matching diagnostic output after genuine initialization. The measurements support that this diagnostic overhead was substantial, especially in call-heavy archive serialization. Device scheduling/power behavior and workload state were not fixed by a laboratory setup; the evidence does not assign every fractional difference solely to the scan.

## Sustained pacing versus computational capacity

After catch-up, traceoff first-epoch windows 300–2300 contain 2100 real completed updates across 104993.546 ms wall time: **20.001 updates/sec**, work mean 35.702 ms elapsed /35.288 ms CPU. Restarted windows 300–1600 contain 1400 updates across 69994.967 ms: **20.001 updates/sec**, work mean 36.171 ms elapsed /35.659 ms CPU. Costs vary over windows: first-epoch game mean 10.414 ms versus restarted mean 11.734 ms; archive mean 23.819 ms versus 23.053 ms. The actual server idle wait now fills genuine pacing gaps rather than the work continually exceeding 50 ms.

This proves headless simulation reaches its deliberately configured 20 Hz target in both map epochs. It does **not** prove 30 FPS or higher rendering. Transient off windows 100/200 produce approximately 56/32 completed updates/sec while catching up owed simulation; they cannot establish sustained capacity. Likewise inverse mean work alone is not a achieved frame rate and ignores maxima, other owners and renderer/client work. Some individual work updates exceed 50 ms; the aggregate pacing result is not a claim that every deadline is met.

Compiler optimization, input/client/GPU/audio rendering performance and connected player workload remain separate unverified work. No extra performance suite or speculative optimization was performed by this analysis; parent proceeds to genuine app/GPU integration.

Complete evidence: `full430-baseline-timing-analysis.json`, `full430-traceoff-timing-analysis.json`, and `full430-controlled-pairs.json` in `build-android-audit/native_frame_timing3/` preserve exact epoch-labelled phases, valid sample counts, source SHA256, runner metadata and calculated paired ratios. The original baseline-only report is preserved beside them. Source code/configuration and device state were not changed by this analysis.
