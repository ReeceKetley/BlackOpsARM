# Pentagon portal readiness recovery — batch48

## Reproduced failure

`x64hi_dxvk47_route` completed glass and the actual upgraded-MP5K purchase,
then exited normally with clean Vulkan validation. Its30PASS/1FAIL remains
a failed regression: an inserted `portal_pack` entered the ordinary portal
destination and did not produce the required pack-room notification.

The trace explains the state transition:

- `PackCheck` inserted four DEFCON pulls before the portal while marking the
  original portal `PLAN_ARMED`.
- A thief round interrupted the last pull. The following Bonfire Sale raised
  DEFCON to5, then expired at275600ms and reset it to1.
- That last pull completed at285150ms after increasing DEFCON only to2.
- The portal retained its earlier armed flag, so it skipped `PackCheck` and
  teleported to a normal laboratory destination at290950ms. The strict portal
  check correctly failed at294000ms. Later recovery still completed the real
  Pack-a-Punch interaction.

## Repair and focused verification

When `PackCheck` inserts prerequisite pulls, it leaves the original portal
unarmed. After the dependencies finish, the existing step driver checks its
current readiness again. A reset can therefore add the remaining pulls before
the portal starts. Gameplay scripts, teleport behavior and the existing portal
PASS/FAIL requirements are unchanged.

`tools/port/check_pack_rearm.ps1` extracts production `PackCheck` and
`InsertStep`. Win32 and x64 pass a script-state fixture reproducing the reset
during dependencies, followed by the remaining pulls and readiness at5. It
also checks failure caps, active Bonfire policy, ordinary portals and floor
recovery ordering. The pinned `17003a7` negative control fails the deferred
portal flag check. Script field reads/entities are fixture stubs; runtime gates
are separate evidence.

All five builds compile0errors with no new pointer-truncation warnings. Fresh
original Win32/high/low/guard routes complete26/26/25/25PASS, all0FAIL/exit0
and all four SaveGame checks. These are headless/god regressions and include
real Pack-a-Punch purchases after normal recovery.

`x64hi_dxvk48_route` selected default pane0 at long range for its glass step.
Five shotgun shots reduced it to11.9health (cracked) when the20second observation
ended, producing one genuine glass-test failure. That attempt was intentionally
stopped at172650ms for a controlled repeat; exit-1 is a deliberate termination,
not an engine crash. Its logs and `intentional-stop.txt` are retained.

`x64hi_dxvk48_pane2_route` explicitly sets `bo1_testclient_glass 2`, the controlled
pane used by the earlier verified glass gates. It shatters pane2 with five real shotgun shots and completes **32PASS/0FAIL/exit0**
through a real `m14_upgraded_zm` purchase, including another thief-round recovery.
It continues30seconds after completion (448550ms to478550ms) and passes all four
SaveGame checks. Sound is enabled; automated god mode is enabled. Actual module
paths confirm the local patched DXVK D3D9 DLL, system Vulkan loader, NVIDIA ICD
and explicit validation/screenshot layers. Presented captures show the active
renderer and shattered pane. **Zero Vulkan validation errors through shutdown.**

All six attempts are retained in `build/run/pack48_results.json`; failed
observations remain failed. All test processes have ended. The full native
warning count remains144 from batch47; this planner change adds no new pointer
warnings. The unresolved water vertex-math audit is separate (WARN0009), as are
the script debugger and wider map coverage. LOW_VA still defaultsON in CMake;
this Vulkan run uses unrestricted `build-x64-hi` with LOW_VA OFF. Batches41-48
remain uncommitted after the pushed17003a7 checkpoint.
