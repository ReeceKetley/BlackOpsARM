# Actual tablet GPU bottleneck profile

2026-10-14. HONOR Pad X8a, Adreno610, Vulkan1.1.128; SDK472/stage44 BuildId b77c7fdaf9eb73f089e0c31a4ecf5aaf44e9fe58. Real rendered interrogation room after canonical ComInit/CA_ACTIVE and touch OK. Rendering/playable Zombies goal remains incomplete.

## Result

The largest measured command groups are end-scene/post-effects and main lighting/reflections/resolves. In the first warmed baseline they account for 70.5% of mean frame span. Depth of field is a substantial contributor: the same-process 1→0→1 test reduces post-effects median from 44.44 to 25.81 ms, then returns to 42.78 ms when restored. Whole-frame medians are 109.63→87.86→106.73 ms. These are elapsed GPU timestamp intervals, not isolated shader busy times.

| Command group | DOF1 baseline ms | DOF0 diagnostic ms | DOF1 restored ms |
|---|---:|---:|---:|
| Whole measured frame | 109.63 | 87.86 | 106.73 |
| Setup | 0.00 | 0.00 | 0.00 |
| UI3D/shadows | 7.37 | 6.60 | 8.00 |
| Depth/FloatZ/clears | 6.33 | 5.88 | 5.89 |
| Lit/reflections/resolves | 32.97 | 31.56 | 32.10 |
| Decals/lights/resolves | 7.67 | 6.91 | 7.18 |
| Emissive | 9.65 | 8.78 | 9.02 |
| End-scene/post-effects | 44.44 | 25.81 | 42.78 |
| Tail/UI | 1.68 | 1.74 | 2.13 |

Baseline 69 valid rows/seq2642–3275; DOF0 121 rows/seq7425–8766; restored 25 rows/seq13199–13429. Only mask0x7f/phase_valid1/all numeric endpoints counted. Baseline excludes first100 sequences after room entry; subsequent captures exclude first50 after clean-room landmark. Rate-limited log samples are not every frame. Each column contains component medians separately, so columns need not sum exactly.

Full-quality room is roughly7–9 FPS; diagnostic DOF0 clean screenshot reports10.5 FPS/95.34ms, restored clean screenshot8.8 FPS/113.44ms. No permanent quality change adopted. Actual console query confirms r_dof_enable1; r_dof_tweak0, sm_enable1, r_scaleViewport1 retained. Initial DOF0 capture/follow1 had console output adding tail cost and is preserved but excluded from fair comparison. IME hidden before final restored sampling. Thin transient warning text remains possible.

Independent SDK471 KGSL samples were99.84–99.90% busy (8 reads), strongly supporting a GPU-side limit alongside these timestamps. This is a system GPU counter, not app-exclusive utilization. The upstream [KGSL counter implementation](https://android.googlesource.com/kernel/msm.git/%2B/60b130a99211a81b1fc406349d25eb220bb8f7bf/drivers/gpu/msm/kgsl_pwrctrl.c) exposes historical busy/total windows; exact HONOR vendor implementation not verified. Clock/max-frequency sysfs reads were denied, so frequency/throttling state is unknown. SDK471 same-process sm_enable0 produced no clear room FPS gain; restored1 before ending that run.

## Implementation and limits

BO1_NATIVE_GPU_PASS_PROFILE defaults OFF; current profile build is ON only for native performance target. Eight safely owned query sets, seven additional timestamps per frame; nonflush reads, no pending reuse, epochs and partial failure release. Legacy g_bo1ProfOn remains false. Existing seven canonical RB_BO1GpuMark call positions provide grouped boundaries. Missing endpoints are n/a. Production ON/OFF compiles and bounded production-provider ownership fixtures passed; actual APK mask0x7f verified. See native_gpu_phase_profile.md and its evidence directory.

Intervals include ordered queue/idle/barrier/resolve gaps and multiple calls; DOF toggle changes the post pipeline, so the18–20ms reduction is not a measurement of one isolated DOF shader. Query writes can perturb overlap; observed ON/OFF FPS is similar but no controlled overhead measurement established. Different sampling windows/thermal/scheduling history constrain precise attribution. Only this frontend room measured; rendered Kino/gameplay remains unverified. No Vulkan/provider upgrade, shader simplification, asset recopy, assertion weakening or broad test rerun.

Next: split p6 around BloomLDR/DOF/other post filters, and p3 around lit/reflected/resolves; inspect real shader work and render-target size against validated BlackOps64 and working DXVK spike. Preserve defaults until measured, visually checked optimization is ready. DOF0 is a diagnostic only.

## Evidence and current process

Raw SDK472 logs/screens/summaries in build-android-audit/native_registry_closure: gpu-full1, gpu-dof-off2, gpu-restored3, restored query/clean2. KGSL data in sdk471-gpu-profile1. Source policy proof in native-gpu-phase-build-policy1; stage44 and package manifests retain exact seven provider checks. Root build9actions/Gradle6s/installSuccess, CA_ACTIVE71352ms, assets reused. PID5463 live, marker42, guardrunning; observer completion is not engine clean exit. SDK471 was intentionally force-stopped after quality restoration to install instrumentation; known canonical rendered quit failure remains unresolved, no guard erased. USB stayawake restored0.
