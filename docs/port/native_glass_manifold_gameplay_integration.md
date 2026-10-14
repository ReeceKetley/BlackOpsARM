# Original glass, contact manifold, client script and gameplay helpers

2026-10-13. Latest full-engine-link207 compiles selected sources but fails
genuine remaining dependencies. Actual Com_Init/Com_Frame unexecuted.

Selected whole cg_shellshock, cg_scr_main, glass_allocator,
phys_contact_manifold, bg_weapons_view and bg_animconditions. Original shellshock/
flash/flare queries, client script/glass-smash handling, allocator cleanup,
manifold feature/polygon logic, weapon bob/sway and animation conditions retained.
No independent algorithm replacement or fabricated gameplay/physics state.

Native weapon sway uses TU-local float min/max comparisons matching the original
Windows macros, including unordered-value selection. Windows original calls are
unchanged; no global macro/compiler/platform flag broadened. Remaining sources
compile unchanged. Assertions remain active.

Evidence:

-206 preserves missing Windows min/max macro errors;207 corrects them and reaches
  real linker dependencies.
- llvm-nm confirms actual CScr_GlassSmash, Allocator::FreeAll, manifold feature/
  polygon and BG_GetBobCycle definitions in bo1_native_database.
- takeover_glass_manifold_gameplay_windows1 passes six whole normal sources on
  Windows x86/x64 exit0, commands/manifests/build-result.json retained.

No new runtime probe or device/asset transfer. Client scripts/glass/allocator/
manifold/weapon/animation-condition native execution and decompiled pointer,
stride, reference/lifetime assumptions remain unverified. Previous arena/queue/
script fixtures keep their documented narrower scopes. Continue genuine audio,
client-name/devgui/profile, server/game/renderer/input/network/cinematic/recovery
services from207; execute authentic startup when the full entry links.
