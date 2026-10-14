# Native parser/compiler, teams, commands and corpse owners

2026-10-13. Actual Com_Init/Com_Frame remains incomplete. No tablet runtime or
asset transfers occurred.

Full bo1_native_database selects six whole canonical sources: cscr_parser,
cscr_compiler, g_team_mp, g_misc_mp, g_cmds_mp and g_player_corpse. Original
script compilation/source diagnostics and game/team/command/corpse algorithms
are retained. FULL excludes only the inherited unavailable SV_Cmd_Argc/Argv
diagnostics because original g_cmds_mp reads real sv_cmd_args with its existing
nesting/index assertions; OFF diagnostics remain. No substitute startup or state.

Native parser includes canonical database/rawfile definitions and standard ctype.
Generated mod-stack source formatting uses bounded real vsnprintf, closes its
va_list and drops with actual Com_Error if output cannot fit. Appended lengths
are accepted only after that check. Native CompileError/CompileError2 diagnostics
terminate their text and close variadic lists before entering original error paths.
Windows secure-CRT/underscore formatter expressions are unchanged.

Evidence under build-android-audit:

- native_registry_closure/full-engine-link257.log retains native declaration/
  formatter failures. link258 compiles corrections and reveals duplicate command
  diagnostics. link259 excludes those FULL diagnostics and reaches genuine links.
- native_registry_closure/parser-compiler-team-corpse-definitions1.log verifies
  actual archive owners for source read/compile diagnostics, team state, server
  arguments and corpse execution. Script compiler whole source compiled in257.
- takeover_parser_compiler_team_commands_corpse_windows1: all six whole normal
  TUs pass x86/x64 exit0 after final source changes.

No initialized parsing/compilation/error handling, mod-stack source generation,
command processing/team/corpse lifecycle has executed. Retained signed-character
classification, compressed source bounds, pointer offsets, VM ownership and new
format/error boundaries require focused runtime/caller audit. Full entry still
needs genuine audio/cinematic/live/profile/game/client/renderer/network/recovery
services. Prior unrelated probes not repeated; no successful startup claim.
