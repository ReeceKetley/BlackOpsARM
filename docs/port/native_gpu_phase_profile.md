# Native GPU phase profile

2026-10-14. Production source frozen for root SDK472/stage44 integration; device execution not performed by this agent. Live SDK471/PID8567 preserved. Rendering/playable Zombies goal remains incomplete.

`BO1_NATIVE_GPU_PASS_PROFILE` defaults OFF under root-owned CMake. At OFF (undefined or0), `Mark` does nothing and each of eight sets allocates/issues exactly the existing four queries. At ON each set owns seven more actual D3D9 TIMESTAMP queries. Root configures ON only for the measured Android provider target; legacy `g_bo1ProfOn` stays false.

The native-only hook at `RB_BO1GpuMark` forwards to the modern provider before the old profiler guard. Existing call positions are reused; all other backend bytes match the saved before source after removing exactly this guarded hook. Windows body and per-view stats reset remain unchanged. No engine renderer/asset/provider/DXVK change.

## Logged measurements

One stderr line per approximately1s of measured closing timestamps starts `BO1_NATIVE_GPU_PHASE`, with `seq`, `total_ms`, `mask`, `phase_valid`, and eight adjacent elapsed intervals `p0_ms` through `p7_ms`:

| Interval | Command group |
|---|---|
| p0: begin to mark0 | Backend setup/pre-view work |
| p1: mark0 to mark1 | UI3D and sun/spot shadow maps |
| p2: mark1 to mark2 | Float Z, clears, main depth prepass, cloak prepass |
| p3: mark2 to mark3 | Main lit, shader warming, IntZ/distortion resolve, reflected, post-resolve lit |
| p4: mark3 to mark4 | Framebuffer alpha, decals, sun/corona, IntZ, dynamic lights, distortion resolve |
| p5: mark4 to mark5 | Emissive |
| p6: mark5 to mark6 | Waypoints, brightening, cloak, end-scene and late post effects |
| p7: mark6 to end | Remaining views, 2D/UI/pre-Present commands |

First occurrence of each mark index wins; later views are included in tail spans. These are command groups, not isolated shader-stage costs. Missing endpoints produce `n/a` for that adjacent interval; no unissued mark is read or represented as zero. Mark timestamps must monotonically increase in index order within the wholeframe begin/end interval. If ordering/range is invalid, `phase_valid=0` and all intervals become `n/a`; wholeframe observation remains valid when its independent begin/end/disjoint/frequency checks pass. Disjoint/zero-frequency/reversed wholeframe results retain the current Disjoint observation and produce no phase line.

## Ownership and verification

Mark validates index, nonzero matching enabled epoch, open set, failure state and pending/eligibility state under the current owner mutex, then checks Issue HRESULT. Initialization failure at any of88 creation positions releases every acquired query. Retire/reset/shutdown release all marks. Pending sets are never reused; pool pressure skips measurement until a set completes. Poll waits for all actually issued marks with GetData flags0 and checks HRESULT. Existing wholeframe Begin/BeforePresent/Presented observation and lifetime APIs remain in use.

Evidence `docs/port/evidence/native_gpu_phase_profile1` contains before sources, implementation script, source freeze hashes, explicit and current-recipe native provider compilation commands/results, and the bounded fixture. ON and OFF native provider compiles pass with Wall/Wextra/Werror. Actual production `.cpp` is included in the host fixture with mocked D3D queries and clock; ON/OFF fixture compiles with W4/WX and runs exit0. Covered: every partial creation failure/release; all8sets pending and ninth attempt without reissue; mark-only pending completion; duplicate/invalid marks; missing endpoints; full seven-mark frame; disjoint/zero-frequency/reversed frame; malformed phase order/range; epoch changes/disabled epoch; issue/read failures; approximately1s rate limit; OFF allocation count/no phase logs. This fixture validates ownership/formatting, not Vulkan performance.

Protected DXVK-Native was inspected read-only. Timestamp Issue adds ordered command-stream work; flags0 polling does not flush. DxvkContext::writeTimestamp does not spill a render pass. Query reset uses host reset or InitBuffer; timestamp writes use ExecBuffer with ALL_COMMANDS stage. Phase profiling adds56query objects and seven timestamp issues per measured frame, plus reset/poll/log overhead, and can perturb pipeline overlap. Elapsed GPU spans include queue/idle/barrier/resolve gaps inside endpoints; they are not GPU busy utilization or shader-only time. Compare ON/OFF same-scene total/FPS and independent KGSL busy evidence before optimization conclusions.

No device commands, master build, package/install, Ledger/STATE writes or protected-provider writes were performed by this agent. Root owns actual APK acceptance and installed recipe proof.

## Root actual APK acceptance

SDK472/stage44 installed with target-scoped profile ON; actual canonical room emits all seven marks (mask0x7f) and valid adjacent intervals. Same-process DOF 1→0→1 confirms a substantial post-effects contribution. Actual measurements, caveats and restored defaults are in native_gpu_bottleneck_profile.md. Current PID5463 live; original source-agent-only statements above describe pre-integration scope.
