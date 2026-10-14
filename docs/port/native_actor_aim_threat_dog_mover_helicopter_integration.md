# Existing actor aim/threat/dog, mover and helicopter owners

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No runtime, assets or
other device transfers this scope.

Full database now selects nine existing whole sources: actor_aim, actor_lookat,
actor_threat, actor_dog_exposed, actor_zombie_exposed, bg_dog_animations_mp,
g_scr_mover, g_helicopter1 and g_scr_helicopter. Actual melee, look-at, threat,
dog animation, mover physics launch and helicopter handler implementations are
connected; no replacement handler callbacks or independent engine copies.

Eight sources unchanged. Android threat debug loop uses engine ARRAY_COUNT rather
than unavailable Windows ARRAYSIZE for the same ten-element array. Windows keeps
original macro. Algorithms, state, trace calls, animation allocation and shutdown
remain original; no fake readiness or successful wait introduced.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link291.log compiles initial selections and
  records ARRAYSIZE failure; link292 compiles correction/mover/helicopter1/zombie
  source; link293 compiles helicopter commands. Full link still fails genuine
  remaining engine/native services.
- actor-aim-threat-dog-mover-definitions1.log confirms real melee/look-at/threat/dog/
  mover/dvar owners and helicopter think/pain/die/controller/method definitions.
- takeover_actor_aim_threat_dog_mover_helicopter_windows1: eight normal whole TUs
  pass x86/x64 exit0 with source manifests.
- takeover_helicopter_commands_windows1: remaining normal whole TU passes both
  Windows ABIs exit0 with source manifests. No unrelated runtime probe repeated.

No initialized actor animation graph, melee/trace, sentient references, mover/
vehicle physics or helicopter handler execution occurred. Dog animation creation
still requires genuine dog_debug and XAnim prerequisites; it is not ready merely
because its code is compiled. Native float-to-integer trajectory conversions,
shared pool/state lifetimes, graph bounds and physics readiness remain runtime
caller gates. Full entry still cannot link or execute Com_Init/Com_Frame.
