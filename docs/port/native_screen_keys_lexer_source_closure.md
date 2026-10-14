# Original screen, key state and menu lexer source closure

2026-10-13. Full-engine-link142 compiles selected sources but still fails linking.
Actual Com_Init/Com_Frame remain incomplete.

Selected whole original cl_keys, cl_scrn_mp, cl_ui_mp, l_script and cg_newDraw_mp.
Full cl_keys replaces the startup target's former reduced source selection so
the canonical key-name/state/command owner is compiled once. OFF selection remains
unchanged. Original screen/load-screen/client-UI state, HUD helpers and menu token
lexer bodies remain canonical; no independent query or fake event/input device
was introduced.

CL_InitKeyCommands FULL diagnostic is excluded only where its actual full original
body now supplies registration. CL_GetLocalClientUIGlobals comes from full original
cg_newDraw_mp rather than a copied query. Key catchers/state/bindings are real engine
state and commands; this does not implement physical OS input or claim readiness.

Native cl_keys consumes the canonical gamepad declarations and excludes unused
XInput implementation types. Windows retains its original backend header. Native
lexer formatting uses the already implemented CRT formatter. CP1252 bytes are
preserved. No shared algorithms, assertion policy, disk/VM widths or memory
initialization were changed in this step.

All five compile ARM; five whole normal Windows TUs pass x86/x64 in
takeover_screen_keys_windows1. Logs140..142 preserve missing formatter declaration,
duplicate inherited key initializer, corrections and final link evidence. No new
runtime fixtures or device/asset transfers were needed for these source changes.

Previous menu token allocation/time lifecycle probe proves its earlier scope only;
it does not establish full lexer/file/preprocessor behavior. Screen/HUD/key state,
registration and actual UI/client/device-input/render loops remain unexecuted.
Next actual global hunk/temp/LargeLocal, screen events/native input, audio/cinematic,
remaining script/client/UI/game/renderer/network owners must be implemented and
selected from link142. Source closure cannot substitute for real startup and
frame/asset loading execution; no successful unsupported endpoint may force it.
