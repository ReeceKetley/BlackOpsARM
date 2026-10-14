# Original string-table, draw, dynamic-piece and ragdoll helpers

2026-10-13. Full-engine-link118 compiles all selected sources but still fails to
link. Actual Com_Init and Com_Frame remain incomplete.

Selected complete original com_stringtable, DynEntity_pieces, cg_draw_mp,
ragdoll_quat and ragdoll_cmds. All five compile ARM unchanged and pass normal
whole-TU Windows x86/x64 compilation in takeover_stringtable_draw_windows1.
No new placeholders, fake registration/state or assertion bypasses were added.
These restore canonical string-table access/checksum helpers, dynamic-piece
Dvar registration, scene-tracer/vertex calls, view-model state, ragdoll math and
command parsing. Their actual engine initialization and runtime remain unverified.
No new standalone tests or device transfers were needed for unchanged selection.

This selection brings original material loading into the retained graph: link118
now explicitly needs D3DXGetShaderConstantTable, D3DXGetShaderInputSemantics,
D3DXGetShaderOutputSemantics and D3DXCompileShader. Earlier discarded-path notes
are historical; these are current link blockers. The loader's shader argument
path consumes CTAB buffer contents, then actual input/output semantics. Its source
compiler wrapper needs real compiled bytecode, diagnostics and constant table
ownership, retaining its original cache/error policy. No empty shaders or
success responses may substitute for these services.

Next implement genuine D3DX shader support beside pinned DXVK Native1.9.2,
preserving tablet Vulkan requirements. For CPU compilation, upstream Wine's
libvkd3d-shader API supports HLSL to D3D_BYTECODE as documented in the primary
[1.11 release API patch](https://list.winehq.org/hyperkitty/list/wine-gitlab%40list.winehq.org/thread/K3BPOBBQ7U73WVEH6WUB3A24UZYK556X/).
Its primary [1.7 announcement](https://list.winehq.org/hyperkitty/list/wine-announce%40list.winehq.org/message/WEGAWNDRKG2FPS3YANBPXVQ4GW6NQWSC/)
describes SM1/2/3 compiler support improvements. This is research evidence for a
candidate CPU compiler, not proof of an Android build or engine shader compatibility.
Pin/review the actual dependency, preserve licensing and validate consumed profiles,
CTAB and semantic behavior against the Windows SDK/oracle before readiness claims.

Other retained dependencies include real audio/cinematic/device, bgame weapon
assets, UI, memory/recovery and remaining game/renderer/input/network owners.
Previous native service probes remain valid only within their documented scope;
source compilation cannot substitute for authentic startup execution.
