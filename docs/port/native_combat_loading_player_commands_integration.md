# Original combat record, loading UI and player script/use/look-at owners

2026-10-13. Full Com_Init/Com_Frame incomplete; production executable unlinked.

Full database target now selects canonical live/live_combatrecord.cpp,
cgame/cg_info.cpp, game_mp/player_use_mp.cpp, game_sp/g_sp_lookat.cpp and
game_mp/g_client_script_cmd_mp.cpp. Original combat-record/stat displays, loading
screen/state, player cursor/use, SP look-at and actual Player_GetMethod table and
callbacks are compiled. No callback substitutes, assertion weakening or changed
accepted reference feature guards.

Eleven native player-script command formatting sites use standard bounded
snprintf with the original fixed capacities, format strings, arguments and
termination assignments. Windows retains each _snprintf call. No variadic ABI
casts, global CRT shadowing or independently copied script implementation added.
Native loading message logs the already formatted message through literal %s;
percent characters in text cannot become a second format-string evaluation.
Windows original logging remains unchanged. Other four selected algorithms
remain unchanged, including validated SP behavior from the oracle.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link357.log records CRT formatter errors;
  link358 compiles final player-script owner; link359 compiles final literal-safe
  loading logging and is latest production link attempt, still unresolved.
- combat-loading-player-command-definitions1.log verifies actual combat-record
  lookup/image, loading UI, cursor hints, SP look-at and Player_GetMethod owners.
- takeover_combat_loading_player_commands_windows1: five normal whole TUs pass
  Windows x86/x64 with source manifests/exit0. Final loading-message source has
  fresh normal both-ABI comparison in takeover_loading_literal_log_windows1.

No initialized player/entity/script VM, loading screen, combat/DDL statistics,
cursor/use trace or look-at consumer executed. Static bounded formatting/source
selection does not prove native script method ABI, command consumers or gameplay.
No new runtime fixture mirroring those formatting calls or unchanged device tests
was needed; no binaries or game assets transferred. Retained decompiled pointer,
stride, field/VM and argument/lifetime assumptions need runtime audit before
certifying initialized callers. Audio/cinematic/renderer/input/debug/Steam/Monkey
and other genuine canonical/native dependencies remain unresolved. Full Com_Init
has not returned and Com_Frame remains unexecuted.
