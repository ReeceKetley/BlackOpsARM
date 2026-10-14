# Native render state, shading and UI integration

2026-10-13. Genuine Com_Init and renderer startup remain incomplete.

Full bo1_native_database now selects eight canonical whole sources: r_state,
r_state_utils, r_shade, r_wind, rb_state, rb_stats, rb_pixelcost and cg_ui_animate_mp.
The original command-buffer globals, draw submission, source-state setup, shader
argument/vertex declaration/index setup, wind, statistics/pixel-cost and UI animation
owners are retained. No fake device, ready flag, Present or substitute renderer.

Android rb_stats includes the existing native CRT integer formatter. Android
r_shade formats the optional profiler pass label using bounded std::snprintf with
the actual array capacity, preserving truncation and termination. Windows retains
the original SDK formatter. All other selected source bodies are unchanged.

Evidence in build-android-audit:

- native_registry_closure/full-engine-link231.log preserves the rb_stats missing
  underscore formatter failure; link232 compiles that correction.
- link233 retains the shader profiler label SDK formatter failure; link234
  compiles the final adapter and fails genuine remaining engine links.
- native_registry_closure/render-state-shading-definitions1.log records actual
  archive definitions of command-buffer state, pixel-cost mode, indexed draws,
  setup pass/vertex declaration, 2D state, wind, primitive stats and UI animation.
- takeover_render_state_shading_ui_windows1: eight whole normal sources pass
  x86/x64 exit0 after all changes.

No initialized command-buffer, shader/device, pixel-cost query, wind or UI animation
path has executed. Optional profiler pointer-pair/combo identities still contain
original 32-bit assumptions; they need native identity handling before relying on
their measurements. Retained decompiled command-buffer copies and GPU lifetime/
recovery also require runtime audit. Full entry still lacks genuine audio,
cinematic, live/profile, game/client, renderer/network and recovery dependencies.
No tablet tests or assets were transferred; prior scoped probes were not repeated.
