# Original weapon and script VM source closure

2026-10-13. Full-engine-link129 compiles selected sources but remains unresolved
at linking. Actual Com_Init/Com_Frame remain incomplete.

Selected complete original bg_misc, bg_weapons, bg_weapons_def, bg_weapons_ammo,
cg_main and cscr_vm. Canonical weapon/item tables and accessors, ammo/model/animation
helpers, client weapon state, full script VM init/settings/notify/execute/error
bodies are present, with original assertions and state publication order. No
independent engine implementation or new unsupported endpoint was introduced.

Three inherited FULL diagnostics Scr_Init/Scr_Settings/Scr_TerminalError are
excluded only because the real full cscr_vm supplies them. OFF diagnostic build
ownership is retained. Scr_InitVariables remains an unresolved owner/diagnostic;
selecting VM init does not imply variable/class/pool dependencies are initialized.

Native VM uses existing SDK-compatible windows.h in place of minwindef.h. Its four
x86 prefetch hints use the actual ARM compiler primitive; Windows preserves its
intrinsic through the local helper. The oracle's Bo1_LoadCodePos four-byte narrow
handle encoding is retained. Android reads that packed stream slot through memcpy
before the existing decoder, avoiding a potentially unaligned typed32-bit load.
Windows keeps the original load. CP1252 source encoding is preserved.

Six whole normal Windows TUs pass both x86/x64 in takeover_weapon_vm_windows1;
final code-position memcpy branch is Android-only and leaves that tested Windows
branch unchanged. All six native sources compile. Logs126..129 preserve missing
minwindef compile failure, three duplicate diagnostics, corrections and final
unresolved link state. No new standalone tests or device/asset transfers.

This is source/platform closure, not full VM runtime validation. Other direct
packed operand accesses, native pointer/handle lifetime/publication, archived
stacks, actual compiled-script loading, notify/execute/recovery require authentic
execution and relevant ARM ABI/alignment audits before readiness claims. A narrow
helper correction cannot establish that every opcode operand is safe.

Next actual movement/vehicle/perk/fire/world Dvar owners and remaining script
variable/compiler/builtin, UI/memory/audio/cinematic/game/renderer/network owners
must be integrated. Link129 exposes those real dependencies; do not replace them
with success flags. D3DX dependencies resolved in previous125 remain resolved;
reuse their scoped passing probes absent related changes. Full startup completion
requires production execution, not Windows compile comparisons.
