# Project Goal

## Current user objective

Replace the Android D3D9 backend with native Vulkan optimized for mobile performance. Test real materials, frontend and gameplay on the authorized tablet. Build a portable backend with mobile-focused capability-driven defaults; preserve intended appearance while allowing small visually acceptable differences rather than requiring pixel identity. Keep Windows BlackOps64 read-only and original retail assets unchanged. Full backend replacement and measured tablet performance remain required, not just a shader or native demo.

Stages: shader/resource foundations, native material draw and presentation, full frontend, mobile pass/lifetime optimization, gameplay/lifecycle/performance validation. Preserve D3D9 reference until native parity and measured gains justify changing the default. Pre-native checkpoint4247b269 at checkpoint/pre-native-vulkan.

Previous headless/import/rendering milestones and evidence remain preserved. Do not repeat asset transfers or broad completed tests.

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
