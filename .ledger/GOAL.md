# Project Goal

## Current user objective

Render the genuine Black Ops Zombies engine and achieve playable gameplay in
the Android APK on the authorized HONOR tablet. BlackOps64 is the validated
Windows reference; original retail assets remain unchanged.

Once rendering works, overlay performance metrics on screen for benchmarking
and tuning: actual FPS/frame time, CPU utilization/time, RAM use and GPU timing
where supported, plus other useful measured engine statistics. Keep measurement
sources/units clear and label unavailable data. Do not fabricate performance or
readiness. Improve demonstrated bottlenecks toward the user's30fps-or-higher
ambition; canonical20Hz server simulation is distinct from rendered FPS.
Preserve the game's appearance while optimizing. User welcomes selected native
Vulkan passes when profiling justifies them and asks to audit DXVK-native too;
prefer measured work reduction over crude visual degradation.

User explicitly changed the active goal on2026-10-14 from headless reliability,
app integration and timing investigation to rendering/gameplay/visible metrics.
Preserve completed headless/asset/import/timing evidence; do not repeat broad
tests or transfer equivalent assets. Prior objectives/handoffs remain archived.

## Current milestone

Actual headless initialization, Kino frames, same-process map_restart and mapped
shutdown pass. Same430 comparison reduces mean server work78.365→35.619ms at
matched15000ms simulation age and restores sustained20Hz before/after restart.
All266assets are genuinely imported through Android SAF, with a committed
receipt and app-accessible roots. Actual APK creates the SDL Vulkan window and
real Adreno610/Vulkan1.1.128 device/resources. Loading Present API returns pass;
rendered canonical Com_Init returns and enters the real input/frame loop.
Visible engine loading UI, title and genuine interrogation-room 3D are proved;
the real frontend client reaches CA_ACTIVE. SDK469 sustains room rendering through
4601 main frames before a separate console callback failure. Render submission
FPS, main/render/process CPU, RSS and genuine GPU span overlay values/toggle are
verified in the actual APK. Complete playable Zombies and stable gameplay
measurements; current owners and actual blocker are in STATE.

## Working rules and references

See STATE.md for current owners/artifacts and ../AGENTS.md for source/ABI rules.
Implement actual providers/canonical engine behavior; no dummy success, fake
readiness, assertion bypass, asset-format alteration or unmeasured FPS claim.
Preserve accepted intentional Windows reference stubs. Dead Ops Arcade excluded.
BlackOps64, DXVK-Native1.9.2/SDL2.30.11 and user's working dxvk spike are read-only.
No Vulkan upgrade, global BO1_PORT_X64, commit/push/reset/clean authorization.
User permits distinct parallel agents; root coordinates source/config/device.
User-accessible source /sdcard/boARM and the already imported app copy must remain
preserved. No recurring full-file hash sweeps or unnecessary asset copies.
