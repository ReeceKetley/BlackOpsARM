# Original map UI, client developer menus, keyboard commands and base64 owners

2026-10-13. Genuine Com_Init/Com_Frame remain incomplete and unexecuted.

Full database target selects five existing canonical sources: cl_devgui.cpp,
ui_commands.cpp, ui_keyboard.cpp, game_mp/ui_gameinfo_mp.cpp and universal/base64.cpp.
Original CL_CreateDevGui/DestroyDevGui, map/arena/custom-game loaders, clan-name
command, keyboard command/state/completion and binary ticket codec now have actual
archive owners. Full-only UI_LoadArenas/Maps/CustomMatchGameTypes diagnostics are
excluded where these bodies replace them. Diagnostic-only configurations retain
their explicit missing-service termination. No successful substitutes added.

Native adaptations in the canonical map source:

- Browser enum table holds 130 native pointers: all-maps entry, up to 128 maps,
  then the required null terminator. Original Windows table remains unchanged.
- Custom arena reads retain the 8192-byte content limit with a separate terminator
  slot. Missing/invalid-size/short-read paths report failure. Every opened handle
  is closed before parsing or returning; only a complete read publishes parsed
  arena state. Windows original body remains unchanged.
- Map, content-pack and custom-game-type counts are checked against the actual
  128/8/32 destination arrays before loops. Invalid counts take real ERR_DROP,
  then abort if it unexpectedly returns. No clamping or fake successful tables.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link338.log compiles five owners and
  records three duplicate inherited diagnostic endpoints. link339.log corrects
  selection and compiles native boundary changes. link340.log is latest production
  attempt after focused fixture configuration and still fails genuine remaining
  engine/platform dependencies.
- ui-map-keyboard-devgui-base64-definitions1.log confirms actual archive owners;
  ui-browser-map-table-size1.log confirms 0x410 bytes (130 ARM64 pointer slots).
- takeover_ui_map_keyboard_devgui_base64_windows1: all five normal whole TUs
  pass Windows x86 and x64, with final source manifests and exit0.
- ui-arena-read-build1.log builds bo1_native_ui_arena_read_tests from the actual
  canonical map source, with function/data GC and UBSan. Isolated FS entrypoints
  use a real temporary FILE for reads/close while controlling advertised lengths;
  linker wraps only UI_ParseInfos to inspect complete payload termination, close
  ordering and publication. Fixture logging does not initialize engine services.
  Production has neither fixture boundaries nor the parser wrap.
- ui-arena-read-push1.log/run1.log: HONOR AKWJ9X5116G04618, ADB-shell UID, exit0.
  Missing file, negative/zero/oversized lengths, genuine short temp-file read,
  five-byte content and exactly 8192 non-null bytes pass ownership/termination/
  metadata checks. Only the new binary transferred; no game assets retransferred.

This fixture does not verify actual engine filesystem initialization, arena token
parsing, retail rawfile import, map-table error recovery, Dvar enum lifetime,
developer menu teardown, clan/profile persistence, keyboard menu/Android IME or
Steam ticket acceptance. The existing keyboard/game/base64 bodies are selected,
not rewritten or certified for all malformed inputs. Retained decompiled
arithmetic, input bounds and initialized consumers need subsequent runtime audit.
Renderer/cinematic/audio, platform/profile/live/session and other real dependencies
remain unresolved. No complete engine executable, Com_Init return or frame proof.
