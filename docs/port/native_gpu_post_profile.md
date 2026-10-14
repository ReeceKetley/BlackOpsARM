# Optional post-effects GPU profile provider

The Android timestamp provider can select seven post-effects boundaries instead
of seven coarse renderer boundaries. This adds no query objects or per-frame
timestamps: the existing eight sets and seven mark slots are reused.

`BO1_NATIVE_GPU_PASS_PROFILE=1` retains profiling. With
`BO1_NATIVE_GPU_POST_PROFILE=1`, `Mark(100..106)` maps to slots 0..6 and coarse
`Mark(0..6)` calls are ignored. Without POST enabled, coarse calls remain active
and post calls are ignored. PASS disabled keeps all marks as no-ops, including
when POST is enabled. Bounds are checked before subtraction, so arbitrary signed
indices cannot overflow normalization.

Each phase record now includes `profile=post` or `profile=coarse` immediately
after `BO1_NATIVE_GPU_PHASE`. Sequence, total, mask, validity and phase fields
retain their meanings. Log parsers must accept this additional field.

Only `src/platform/android/performance_gpu_overlay.cpp` changed in this provider
subtask. It retains LF line endings. Its archived pre-edit SHA256 is
`EFC2485C3FD7813A1BAABE0512009D51348C5FF2A40E283A1B152EE1D2BA0C47`;
the final SHA256 is
`D2601A3020992ACE745F67F6062AE7AF70C54F585AD3D3E5054157060FA81C8E`.
The query allocation, retirement, nonflush polling, pending-set ownership,
epoch checks, lifecycle cleanup and first-mark-only behavior are unchanged.

## Targeted verification

Evidence is in `evidence/native_gpu_post_profile1`. Run from BlackOpsARM:

```
python docs/port/evidence/native_gpu_post_profile1/verify.py
```

All four PASS 0/1 × POST 0/1 combinations pass actual Android provider compilation
with `-Wall -Wextra -Werror`, and production-provider owner fixtures compiled
with MSVC `/W4 /WX` and executed successfully. Exact commands and logs are saved
per combination; `results.json` contains their exit codes and log checks.

The added fixture cases prove wrong-mode and invalid indices issue no timestamp
or consume a slot; all seven accepted indices map to the expected slot, issue
once despite duplicates, and retain order. Existing cases cover each partial
allocation failure, release of all 88 objects (32 with PASS off), filling all
eight pending sets without ninth-set reissue, pending mark reads, epoch changes,
disabled observation, issue/read failures, disjoint/zero-frequency/reversed
timestamps, missing boundaries, malformed mark order and report throttling.
OFF combinations emit no phase record; enabled combinations use only the
selected profile name.

These are provider compile/ownership fixtures. Root integration owns canonical
post-effects marker placement, build flags and actual tablet measurements; this
report does not claim actual post-effects execution or GPU costs. Timestamp
intervals retain queue/gap costs and profiler intrusion caveats from the coarse
profile report.

## Root APK acceptance

SDK473–475 actual post-profile marks all valid. Current installed475/reused476 has POST/PASS ON for diagnosis; actual grouped results and ownership/lifecycle caveats are in native_gpu_post_followup.md. No extra queries; legacy profiler false.
