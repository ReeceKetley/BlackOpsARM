# Native script, spawn, client, notification and post-effect owners

2026-10-13. Actual Com_Init and Com_Frame remain incomplete. No tablet runtime
or asset transfer occurred.

Full bo1_native_database selects eight whole canonical sources: g_scr_main_mp,
g_spawn_mp, g_client_mp, demo_ui, xanim_clientnotify, r_add_cmdbuf, r_draw_lit and
rb_postfx. These connect original server script binding/notifications, entity
spawning, client lifecycle, demo controls, animation notification ownership,
delayed draw commands and lit/post-effect algorithms. No duplicate implementation,
dummy startup, successful unsupported endpoint or readiness substitute was added.

Android game-type script basename normalization uses the existing I_strlwr
engine helper in place of the Windows-only _strlwr. It lowercases the engine's
ASCII names in place. Windows retains its CRT call. No locale-general equivalence
is claimed. Other selected source bodies are unchanged.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link240.log preserves the missing _strlwr
  declaration failure; link241 compiles the correction and reaches genuine links.
- native_registry_closure/script-spawn-notify-postfx-definitions1.log confirms
  actual native archive definitions of g_scr_data, Scr_AddEntity, Scr_Notify,
  spawning, ClientConnect, demo dialog, animation list, delayed commands, lit
  drawing and depth-of-field policy functions.
- takeover_script_spawn_client_notify_postfx_windows1: all eight whole normal
  sources pass x86/x64 exit0 after the final source change.

No initialized script execution/binding, spawn/client lifecycle, animation notify
consumption, demo or GPU post-effect path has executed. Original native pointer,
layout, concurrency, ownership and recovery paths still need runtime validation.
The full entry still needs genuine audio/cinematic/live/profile/game/client/
renderer/network/recovery dependencies; source compilation does not prove startup.
Prior unrelated scoped probes were not repeated.
