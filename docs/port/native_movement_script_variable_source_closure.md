# Original movement registration and full script-variable ownership

2026-10-13. Full-engine-link132 compiles the expanded selected sources but still
fails linking. Actual Com_Init/Com_Frame remain incomplete.

Selected complete original bg_jump, bg_mantle, bg_dtp, bg_dog, bg_vehicle_anim,
bg_perks, bg_fire, bg_wind and cscr_variable. Movement/weapon policy Dvars and
their algorithms remain canonical; no forced registration/active state was added.
These are real bodies requested by original BG_RegisterDvars, not substitutes.

FULL entry removes its former limits-only cscr_variable source, using the full
database owner for actual variable/class/array/refcount/init/dump bodies and the
same canonical limits globals. OFF keeps its previous reduced selection. A missing
cctype declaration was supplied; no shared algorithms changed. Three FULL
diagnostics Scr_InitVariables/Scr_DumpScriptThreads/Scr_DumpScriptVariablesDefault
are excluded only where the complete original source supplies them.

All nine native sources compile; all nine whole normal Windows TUs pass x86/x64
in takeover_movement_variable_windows1. Logs130..132 preserve missing tolower
declaration, duplicate inherited diagnostics, corrections and final link evidence.
No new runtime fixtures or device/asset transfers were needed for source selection.

Inspection confirms original Scr_InitVariables uses typed VariableValueInternal
size and canonical parent/child limits, real Scr_AllocVarsMem, save maps, debug
ref/leak storage and original initialization order. This is not runtime proof:
actual allocation/publication, class maps, reference reclamation, archived stacks,
native handles and VM execution require authentic engine state and focused checks.
Existing script memory/string probes cover their earlier scope only. Other packed
VM operand accesses still need alignment/lifetime audits before broad readiness.

Current remaining link owners include actual UI/client/platform/gamer-profile,
global hunk/LargeLocal and recovery, script parser/compiler/builtins, native audio/
cinematic device/IO, game/renderer/network services. Movement owners now selected
do not establish full movement/gameplay behavior. Continue real source/platform
integration and production startup; no placeholder, assertion bypass or synthetic
ready state may be used to force closure. D3DX services remain resolved; do not
repeat those passing probes absent related changes.
