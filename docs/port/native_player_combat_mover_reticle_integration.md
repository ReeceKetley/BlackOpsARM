# Native player, combat, mover, weapon and reticle owners

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No tablet run or asset
transfer occurred.

Full bo1_native_database now selects seven whole original sources: g_active_mp,
g_combat_mp, g_mover, g_weapon, cg_draw_reticles, cg_draw_names and
cg_draw_indicators. Actual player movement/update, trigger handling, combat,
mover/constraint release, weapon parameters/muzzle computation and HUD indicator
owners replace missing links. No independent gameplay implementation or readiness
substitute was added.

Android player movement clears both pmove_t instances with sizeof(*pm), replacing
the retained 0x258-byte original clears. Native runtime pointer/layout growth and
additional fields must not leave stale movement state. Windows clears are unchanged.

Clang rejected a chained comparison in the original reticle arc expression. The
same expression is present in protected BlackOps64. Android adds parentheses to
preserve its actual C++ evaluation: (anglePlusEpsilon < leftArc - rightArc) compared
against angleMinusEpsilon. It does not invent the missing conjunction suggested
by an old comment. Retail arc behavior still needs a separate audit; this change
only makes reference semantics explicit. Windows expression remains unchanged.

Evidence in build-android-audit:

- native_registry_closure/full-engine-link235.log retains the reticle comparison
  compile diagnostic. link236 compiles the correction and native movement clears,
  then fails genuine remaining dependencies.
- native_registry_closure/player-combat-reticle-definitions1.log records actual
  archive owners for G_Damage, G_DoTouchTriggers, g_pmove, DeleteConstraint,
  muzzle/weapon helpers, night vision, crosshair scan and grenade indicators.
- takeover_player_combat_mover_reticle_windows1: seven whole normal TUs pass
  x86/x64 exit0 after final source changes.

Player/combat/mover/weapon and UI runtime, movement clearing, physics ownership
and HUD behavior have not executed in the initialized game. Retained decompiled
sizes/offsets and concurrency still require runtime audit. Full entry still lacks
real audio/cinematic/live/profile/game/client/renderer/network/recovery services.
No complete startup or frame result; prior unrelated probes were not rerun.
