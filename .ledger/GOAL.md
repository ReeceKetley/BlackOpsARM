# Project Goal

## Current user objective

Milestone reached (2026-10-15): SDK626 sustains16.1356FPS at720p over179.107seconds (290profiles, sixscreenshots).1080p15FPS and fullgameplay validation remain unproved. Requested goal: reach sustained15+FPS on the HONOR tablet in the native Vulkan menu scene, preferably1080p with720p acceptable. Continue measuring CPU/GPU costs, preserve recognizable appearance, and verify real sustained submissions. Separate performance build is an experiment; retain the existing diagnostics build.

Status 2026-10-15: first menu performance milestone reached at the lower end of the reported baseline range: native1080p8.061FPS and720p8.707FPS, same engine binary. Current evidence and limits are in STATE and docs/port/native_vulkan_d3d9_parity.md. This does not complete the broader gameplay/lifecycle/backend objective.

Current requested benchmark (2026-10-15): first match the user-reported D3D9/DXVK 7.8ÃƒÂ¢Ã¢â€šÂ¬Ã¢â‚¬Å“11 FPS in the actual main-menu 3D scene on the HONOR tablet. Prefer 1080p; 720p is acceptable if necessary. Establish actual render resolution, measure CPU and per-pass GPU costs, implement evidence-based native Vulkan efficiency improvements and verify sustained performance and intended appearance. Do not claim parity from an isolated screenshot, loading frame or microbenchmark. The earlier 30 FPS aspiration is not this first milestone. User explicitly permits efficient native Vulkan replacements for post-processing and other rendering effects; pixel-perfect D3D9 matching is not required. Preserve the recognizable Black Ops visual identity, with actual tablet screenshots validating lighting, shadows, colour grading and atmosphere. Original retail assets remain untouched; native effect implementation and pass structure may differ.

Replace the Android D3D9 backend with native Vulkan optimized for mobile performance. Test real materials, frontend and gameplay on the authorized tablet. Build a portable backend with mobile-focused capability-driven defaults; preserve intended appearance while allowing small visually acceptable differences rather than requiring pixel identity. Keep Windows BlackOps64 read-only and original retail assets unchanged. Full backend replacement and measured tablet performance remain required, not just a shader or native demo.

Stages: shader/resource foundations, native material draw and presentation, full frontend, mobile pass/lifetime optimization, gameplay/lifecycle/performance validation. Preserve D3D9 reference until native parity and measured gains justify changing the default. Pre-native checkpoint4247b269 at checkpoint/pre-native-vulkan.

Previous headless/import/rendering milestones and evidence remain preserved. Do not repeat asset transfers or broad completed tests.

## Current milestone

The requested native Vulkan menu performance milestone is achieved at720p: SDK62616.1356FPS sustained over179.107seconds, reported in docs/port/native_vulkan_15fps.md. Remaining work is1080p GPU efficiency and gameplay/lifecycle verification. The milestones below are historical prerequisites.

Actual headless initialization, Kino frames, same-process map_restart and mapped
shutdown pass. Same430 comparison reduces mean server work78.365ÃƒÂ¢Ã¢â‚¬Â Ã¢â‚¬â„¢35.619ms at
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
