# Native missile, item, snapshot and tessellation draw owners

2026-10-13. Actual Com_Init remains incomplete. No tablet run or asset transfer.

Full bo1_native_database now selects eight whole canonical sources: g_missile,
g_items, g_sp_player_state, cg_snapshot_mp, rb_tess, r_draw_bsp, r_draw_xmodel and
r_draw_staticmodel. This connects actual game updates/SP state, client snapshot
cleanup and dispatch into BSP/model draw behavior, retaining original algorithms
and ownership. No dummy initialization or substitute game/render implementation.

Android BSP drawing uses the compiler's real read-prefetch builtin with locality1
at the original address. Windows retains its original intrinsic. Other selected
source bodies are unchanged. Existing assertion and snapshot copy policies remain.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link237.log compiles the initial five whole
  sources; link238 preserves BSP prefetch declaration failure; link239 compiles
  the corrected draw sources and fails genuine remaining links.
- native_registry_closure/missile-item-tess-draw-definitions1.log verifies actual
  native archive definitions of G_RunMissile, G_RunItem, G_SP_PlayerBuiltinState,
  CG_ShutdownEntity, triangle tessellation and BSP/dynamic/static-model draws.
- takeover_missile_item_snapshot_tess_draw_windows1: all eight whole normal TUs
  pass x86/x64 exit0 after final source changes.

No initialized missile/item simulation, SP state lifecycle, client snapshot
transition or GPU drawing has executed. BSP next-element prefetch relies on the
original draw-list contract; callers/bounds and retained native pointer/stride
paths still require runtime audit. Full entry still requires real audio/cinematic,
live/profile, game/client, renderer/network and recovery dependencies. No complete
Com_Init or Com_Frame result; prior unrelated scoped tests were not repeated.
