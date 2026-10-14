# Existing actor core, animation, navigation and event integration

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete; no tablet run/transfers.

Full bo1_native_database selects seven existing whole sources unchanged: actor_mp,
actor_generic, actor_animapi, actor_navigation, actor_exposed, actor_events and
actor_event_listeners. Real actor initialization/allocation/indexing/pile-up,
generic state, animation, navigation, exposed operations and events are connected.
No independently reimplemented AI, dummy readiness, skipped assertions or source
body changes. Protected Windows reference untouched.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link283.log compiles selected whole sources,
  then fails genuine remaining links.
- actor-core-anim-events-definitions1.log confirms actual native G_InitActors,
  G_GetActorIndex, Actor_ClearPileUp, animation and event archive owners.
- takeover_actor_core_animation_navigation_events_windows1: seven whole normal
  TUs pass x86/x64 exit0. Source hashes/manifests retained by the helper.

No initialized actor allocation/extended sentient storage, animation/path/events
or AI runtime has executed. Existing sizeof-based copies are retained; source
inspection is not runtime proof of native layout, handle publication, virtual
physics objects, pool/reference cleanup or concurrency. Full entry still needs
other omitted original owners and real native services. No gameplay/Com_Init/
Com_Frame completion claim; prior unrelated scoped probes not repeated.
