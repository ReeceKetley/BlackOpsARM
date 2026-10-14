# Original rope/flame rendering and full shared utilities

2026-10-13. Latest full-engine-link197 compiles new sources but fails genuine
remaining dependencies. Actual Com_Init/Com_Frame/rendered execution incomplete.

Selected complete cg_localents, cg_players_mp, r_rope_render, r_mapkit_render,
flame_system and flame_class_stream. These own original tracer/player draw,
rope information/vertex generation, existing mapkit rendering and flame generation/
rendering. Existing algorithms/state/assertions retained; no dummy geometry or
fake renderer readiness supplied.

Full canonical q_shared now selected in native database and removed from partial
startup selection, preserving one shared utility/thread-data owner. Complete
character/string/info/endian/math/allocation helpers retain original implementations
and previous native adaptations. Android uses existing native legacy CRT adapter
for _atoi64/underscore formatter calls; normal Windows paths unchanged. Independent
focused thread-data targets retain their own explicit selection.

Evidence:

-195 compiles six unchanged original renderer/game sources and reaches real link
  failures.196 exposes full q_shared native CRT dependencies;197 resolves compile
  errors with real adapter and reaches genuine remaining links.
- takeover_rope_flame_qshared_windows1 passes seven whole normal sources on
  Windows x86/x64 exit0 with commands/manifests/build-result.json. Final native-only
  CRT include added after that terminal result; Windows implementation unchanged.

No new runtime probes or device/asset transfers. Rope/flame/player/tracer/mapkit
execution and native decompiled pointer/stride assumptions remain unverified.
Full shared utility selection does not certify every helper or expand previous
focused runtime proof. Continue actual raw-text/color/pixelcost/outdoor/server/
trace/client/game/renderer/audio/cinematic/input/network/recovery owners shown
in197; then run genuine startup and pending initialized native lifecycles.
