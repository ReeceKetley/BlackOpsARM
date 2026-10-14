# Original static collision, sun rendering and shared menu/time policy

2026-10-13. Full-engine-link212 compiles selected sources but fails genuine
remaining dependencies. Actual Com_Init/Com_Frame/rendering unexecuted.

Selected whole cm_staticmodel, r_sky, r_sunshadow, rb_corona and r_vertexstream2.
Canonical common menu classification (Com_IsRunningMenuLevel/Com_IsMenuLevel),
sound time scale and frame-time reset bodies now included by native boot. Their
real Dvar/state/clock behavior remains original; no forced menu/time/game state.

Native sun-shadow maximum preserves original Windows floating-point comparison,
including unordered selection. Native corona GPU query loop uses actual zero-
duration Sleep/yield while retaining real query/result/error handling. Windows
original macro/Sleep calls unchanged; no synthetic completed query/device result.
Existing assertions retained.

Evidence:

-211 preserves native Windows max/Sleep errors;212 corrects those and reaches
  genuine links. Named collision/sun/corona/vertex/menu/frame/sound-time gates are
  no longer unresolved in212.
- takeover_static_sun_menu_time_windows1 passes six whole normal TUs (five selected
  plus common) on Windows x86/x64 exit0, commands/manifests/build-result.json retained.

No runtime probes or device/asset transfers. Actual collision/world/sun/view/GPU,
menu classification, sound scale/frame clock and pointer/stride paths unexecuted.
Previous native clock/yield proof remains narrower than actual renderer lifecycle.
Continue genuine EntHandle/server-state/audio/cinematic/client/game/renderer/input/
network/recovery owners retained in212; run authentic startup once entry links.
