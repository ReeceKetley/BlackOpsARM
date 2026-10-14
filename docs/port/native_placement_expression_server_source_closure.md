# Original placement, expression and UI-server helpers

2026-10-13. Full-engine-link148 compiles selected sources but still fails linking.
Actual Com_Init/Com_Frame remain incomplete.

Selected complete original screen_placement, com_expressions and ui_server.
They supply canonical placement globals/transforms, expression init/free/parse
ownership and server-list/tooltip logic. Existing algorithms and assertions remain.
Native UI-server formatting uses the existing real CRT adapter; CP1252 source
encoding is preserved. No additional resource/backend implementation or readiness
flag was invented.

FULL Expression_Init diagnostic is excluded only because the actual original
source owns it. OFF diagnostic build remains as before. No new successful or
unsupported placeholder was introduced to force linking.

All three compile ARM; three whole normal Windows TUs pass x86/x64 in
takeover_placement_expression_windows1. Logs147/148 preserve inherited expression
duplicate, correction and unresolved link evidence. No new runtime probes or
device/asset transfers were needed for these source changes.

Original placement/expression/server-list execution, rendered UI and genuine
hunk init are still unverified. Global hunk init retains real filesystem,
reflection-Dvar, tracking/clear and command prerequisites; no synthetic state
was supplied to claim initialization. Remaining actual audio/cinematic/physics,
screen/event/input/client/UI/script/game/renderer/network owners still prevent the
production entry from linking. Continue their real integration and then execute
authentic startup; scoped compile comparisons cannot prove the full objective.
