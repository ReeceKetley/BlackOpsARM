# Original LAN, demo fields, SP HUD and shared SP map policy

2026-10-13. Latest full-engine-link179 compiles added sources but fails linking
remaining genuine engine dependencies. Actual Com_Init/Com_Frame unexecuted.

Selected complete canonical cl_ui_pc_mp, demo_version and cg_sp_hud. These own
original LAN server-list/ping/status/filter operations, demo netfield lookup and
SP HUD/fade state. SP map-policy helpers in canonical common.cpp are now included
by the native boot selection: Com_IsSPFrontEndLevel, Com_IsSPFrontEndRunning and
Com_IsSPLevel. Original bodies consume actual bo1_zombies/zombiemode/sv_mapname
Dvars; no forced SP readiness, menu state or substitute helper implementation.
Normal Windows inclusion and algorithms remain unchanged.

Android LAN friends formatting uses bounded std::snprintf. Required output length
is checked before advancing the cursor; truncated text stays within its128-byte
buffer. Empty lists are initialized to an empty string and do not predecrement
the buffer start; ellipsis append is bounded. Windows retains original formatter
and branch behavior. This corrects native source-level memory safety risks;
actual initialized friends/server-list execution remains untested.

Evidence:

-177 preserves missing sprintf_s compilation error;178 corrects it and reaches
  real link failures.179 compiles final native empty-string initialization.
- LAN_GetServerCount/LAN_MarkServerDirty, demo field lookup, Com_IsSPLevel and
  cg_defaultFadeScreenColor are no longer unresolved in the final link.
- takeover_lan_demo_sp_hud_windows1 passes four whole normal sources (including
  common.cpp) on x86/x64 exit0 with manifests/commands/build-result.json. Final
  native-only buffer initialization leaves those Windows branches unchanged.
- Previous milestone's final SDL_APP_TERMINATING and focus-return waiting edits
  compile in177 onward. sdl-lifecycle-build4/run3 pass on HONOR UBSan with actual
  queued key/background/foreground/termination/quit events, exact-once consumer
  delivery, owner restrictions and missing-window rejection. Positive focus/
  window/Activity behavior remains unverified; logical focus-wait guard compiles
  but cannot run without a real attached video window.

Only changed SDL probe executable transferred; existing SDL runtime/assets reused.
Selected demo netfield offsets and SP HUD native pointer/math paths are not
blanket-certified by compilation. Runtime layout/serialization/graphics behavior
requires actual integration evidence. Continue real gameplay/client/UI/physics/
renderer/script/platform links, then bind actual SDL window/input owner and run
the production startup. No fake server/friends/game/renderer state supplied.
