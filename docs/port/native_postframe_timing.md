# Actual frame measurements and postframe split

Updated 2026-10-14. Actual full429 tablet measurements establish that the principal measured cost is CPU work inside server postframe. The added second-level phase instrumentation is compiled and ready for the next combined production executable; those narrower phases have not yet executed on the tablet.

## Actual evidence

Source: `build-android-audit/native_registry_closure/full-engine429-honor-restart-timing1.log`. Its unchanged SHA256 and parsed aggregate values are saved in `build-android-audit/native_frame_timing2/full429-timing-analysis.json`.

At completed server frame 200, the preceding 100-update window took **7393.122 ms**, or **13.526 completed updates per real second**. It advanced 5000 ms of simulation. Actual inclusive work consumed 7362.230 ms elapsed and 7327.764 ms calling-thread CPU: **73.622 ms elapsed / 73.278 ms CPU per completed update**. Postframe accounted for 6245.906 ms elapsed / 6218.018 ms CPU, or **62.459 / 62.180 ms per update**, approximately **84.8% of measured work**. Actual `G_RunFrame` used **10.975 ms elapsed / 10.916 ms CPU per update**.

These are actual O0/debug/function-instrumented/UBSan development executable numbers, not release-game performance or rendered FPS. Initial game/asset loading is excluded from these windows. Neither 5.9 updates/sec from whole-process runtime nor 7.4 seconds per individual update describes this stable active window.

| Window ending at frame | Completed updates in window | Updates / real second | Work elapsed / update | Work CPU / update | Post elapsed / update | Game elapsed / update |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 97 | 13.577 | 73.283 ms | 72.931 ms | 62.186 ms | 10.897 ms |
| 200 | 100 | 13.526 | 73.622 ms | 73.278 ms | 62.459 ms | 10.975 ms |
| 300 | 100 | 12.769 | 77.741 ms | 74.775 ms | 65.973 ms | 11.575 ms |
| 400 | 100 | 12.067 | 82.380 ms | 76.930 ms | 68.900 ms | 13.199 ms |
| 500 | 100 | 13.526 | 73.615 ms | 73.274 ms | 62.430 ms | 10.994 ms |

The frame-100 window excludes frames 1–3 because they were reported individually. Windows 300/400 show increased CPU cost and elapsed descheduling/stall overhead; no cause for that variation is established. The two approximately equal stable windows 200 and 500 corroborate the baseline.

The main frame at wake index 200 measured 7392.120 ms elapsed / 1439.760 ms main-thread CPU. Its one `SV_Frame` call scheduled **100** wakes and took 7390.015 ms elapsed; it was waiting for the server and assisting, not executing 100 game updates on the main owner. Actual completion polls totaled 6675.615 ms elapsed / 749.178 ms CPU; assistance totaled 298.853 / 291.329 ms. These scopes overlap inclusively with `SV_Frame`; their elapsed totals must not be added across threads or treated as extra server work.

The associated canonical hitch warning reports 7393 ms, matching the independently measured previous main-frame duration of 7392.137 ms and batch count 100. This confirms the source interpretation: the hitch is a **catch-up batch**, driven by the unchanged dedicated 5000 ms clamp and 50 ms server timestep. The server-post CPU cost is what makes this measured batch exceed real-time simulation pace; pure sleep/idle time is a small part of the stable window.

## Where postframe work occurs

Canonical `SV_PostFrame` runs the script debugger, client timeouts, `SV_SendClientMessages`, and the dedicated Steam periodic auth check. `SV_SendClientMessages` includes entity diagnostics, animation-command collection, client snapshot build/send work, and **unconditional archive serialization while SS_GAME**, followed by archive ring-buffer publication. A headless server with zero active clients still performs genuine archive work. Skipping that owner to manufacture a better timing number would change behavior and is not an acceptable fix.

The actual archive implementation uses cached snapshot history, serializes MatchState, clients/playerstate and entity deltas, and maintains the original caches and end marker. At present, the measurements establish dominant **postframe**, not yet which of these inner owners dominates.

## Added measurement boundaries

Android-only aggregate categories now split the existing server postframe into:

- `post_debugger`, `post_timeouts`, and `post_auth`: actual canonical calls.
- `snapshot_send`: inclusive `SV_SendClientMessages` duration.
- `snapshot_far_check` and `snapshot_anim_collect`: canonical diagnostic and animation collection work.
- `snapshot_build_clients` and `snapshot_transmit_clients`: actual outer client loops, including their original validation, fragment and voice behavior.
- `snapshot_build`, `snapshot_write`, and `snapshot_transmit`: actual client snapshot build, serialization and message transmit functions when they execute.
- `archive_serialize`: existing MSG_Init plus actual `SV_ArchiveSnapshot` call.
- `archive_commit`: original server-header publication, archive overflow/error handling, ring-buffer copy/accounting and g_archivingSnapshot reset.

The existing per-thread monotonic/CPU aggregates and rate limits are reused. Nothing measures or logs every NetField or every asset. No archive, timestep, client selection, Steam, error/assertion, cache, asset or ownership algorithm has been changed. All scopes remain inclusive, with missing clocks explicitly marked unavailable.

## Compiler tracing is a separate candidate cost

The current production CMake cache is Debug with `-O0 -g`. Engine targets retain `-finstrument-functions`, UBSan and no sanitizer recovery. The actual `__cyg_profile_func_enter` implementation scans **14 startup-target entries for every instrumented function**, even when an ordinary snapshot/message helper cannot match any startup target. Its ARM64 object disassembly confirms the 224-byte target table (14 pairs), an O0 loop, 0x170-byte stack frame and inserted UBSan checks inside the hook. Saved evidence: `current-trace-hook-disassembly.txt`.

That is a plausible significant overhead in call-heavy delta encoding, but source/disassembly alone cannot quantify it or establish that it explains the measured 62 ms postframe cost. The new timing helper itself excludes compiler function tracing and samples coarse phases only.

After the next actual run identifies the archive/client/post owner, a controlled comparison can keep engine sources, assets, map, canonical timestep, O0 and UBSan identical while changing **only whether the startup trace hook scans after actual Com_Init has returned**. A runtime trace gate with documented baseline/disabled windows can avoid retransferring assets or rebuilding the whole engine twice; a separate executable variant is also possible. Disabling a diagnostic scan is distinct from deleting engine/archive work. Startup trace coverage can stay active through actual initialization. Compare CPU and elapsed phases before drawing a conclusion. Evaluate compiler optimization in a separate comparison so the effects of tracing and optimization remain distinguishable. This report makes no claim that either change has been applied or measured.

## Verification and current boundary

Actual production NDK30/API28 compile commands replayed to isolated objects for sv_main_mp, sv_snapshot_mp and common (the latter also includes the extended inline timing header): all exit 0, O0/function tracing/UBSan retained. Normal Windows x86 and x64 whole translation units for the two changed server sources also pass with the established comparison options. Existing Windows branches and canonical algorithms are preserved; protected BlackOps64 and DXVK were not edited. No standalone framework, broad repeated suite, full build, device transfer or device run was performed by this agent.

Frozen hashes and byte-preserving before snapshots/diffs are in `build-android-audit/native_frame_timing2/source-manifest.json`. Parent agent owns the next combined build, production run and Ledger/STATE update. Actual full429 timings are evidence for the original broad phases; the newly added postframe subdivisions require the next executable before making any inner-owner performance claim.

## Actual full430 baseline follow-up

The new postframe subdivisions have now executed during a real successful Kino map/restart/quit run. Both map epochs show archive serialization approximately 64 ms per update, about 96.1% of postframe, while client loops and archive publication/copy are small. See [Same-executable startup trace comparison](native_trace_frame_comparison.md) for exact epoch-separated windows and the controlled comparison boundary. This supersedes the pending inner-phase evidence above; the trace-scan-disabled comparison remains unproved until actual results arrive.

## Completed same 430 trace comparison

Both actual 430 runs pass real restart and mapped quit with exit 0 and clean guards. The exact first-map 15000 ms comparison reduces archive serialization 64.160→23.799 ms and work 78.365→35.619 ms by gating the continuing startup trace scan after genuine Com_Init. Sustained traceoff windows reach canonical 20 Hz before and after restart; this is not rendered 30 FPS proof. See [completed comparison](native_trace_frame_comparison.md) for paired epochs/clock differences, transient exclusions, variability and complete evidence.
