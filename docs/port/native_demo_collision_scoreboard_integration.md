# Original demo, collision, performance, scoreboard and water loader integration

2026-10-13. Full-engine-link189 compiles selected sources but fails genuine
remaining dependencies. Actual Com_Init/Com_Frame remain unexecuted.

Selected complete demo_common, demo_playback, cg_perf, cg_colltree,
cg_scoreboard_mp and r_water_load_obj. Original demo state/queries, client collision
world/nodes, performance state, scoreboard Dvars/logic and water setup loader now
retained. Assertions and existing algorithms preserved; no synthetic state/data.

Native adaptations:

- Collision source's Windows intrinsics header scoped to Windows. Checker caller
  diagnostic uses actual dladdr module base, or preserves real absolute address
  when lookup fails. A native-only flag labels the diagnostic module-relative or
  absolute correctly. No Windows module handle fabricated; tree/checking logic
  unchanged. Private diagnostic field is not serialized asset/game state.
- Two platform-neutral GPad queries moved to canonical client/cl_gamepad header;
  win_gamepad consumes that header instead of redeclaring them. Native demo uses
  those declarations without XInput state implementation. Native controller
  provider remains required; no fabricated buttons or enabled-device flags.
- Screenshot create time uses actual std::time(nullptr) on Android and retains
  original _time64 on Windows. Screenshot/device/game state execution unverified.

Evidence:

-186 preserves missing intrin/XInput/time calls;187 corrects those and catches
  Windows module lookup.188 compiles real native lookup and reaches linker gates.
-189 compiles final native diagnostic labeling and reaches genuine link failures.
- takeover_demo_collision_scoreboard_windows1 passes seven whole normal Windows
  TUs (six selected plus original win_gamepad) on x86/x64 exit0, commands/manifests/
  build-result.json retained. Final native-only private diagnostic/label edits
  leave Windows code/layout unchanged.

No runtime probes or device/asset transfers. Native demo offsets/serialization,
collision-tree/world ownership, scoreboard/performance, screenshot and water
loader execution remain unverified; compilation does not certify all decompiled
pointer/stride assumptions. Previous focused tests retain only their proven scope.
Continue actual audio/cinematic/gamepad/input/live/profile/server/game/client/
renderer/network/recovery owners shown in189, then execute authentic startup.
