# Original client, message and ragdoll source closure

2026-10-13. Actual Com_Init remains incomplete. Full-engine-link113 compiles all
selected sources but still fails to link; no new unavailable endpoints were added.

Selected complete original cl_main, cg_main_mp, msg, sv_msg_write, ragdoll and
r_utils. These supply real clientUIActives, CL_GetLocalClientConnection, broader
client-game state, reliable command serialization, ragdoll pool/body validity and
asset hashing. Existing assertions/state allocation remain; selecting definitions
does not prove their initialization or game/renderer behavior. Canonical Windows
reference files remain protected and unchanged.

Sixty-six signed32 field-code switch literals in sv_msg_write now use their
negative decimal values (for example0xffffff95 is -107), preserving bit patterns,
switch dispatch and wire-format algorithms. CP1252 source encoding is preserved.

MSG_RoundFloatToInt retains original x87 Windows x86 and SSE Windows x64 paths.
Android implements the validated x64 path's double bias followed by float rounding
using the active rounding mode. rint preserves fractional inexact reporting;
nonfinite or unrepresentable conversion raises FE_INVALID and returns INT_MIN,
matching the consumed conversion contract without an undefined C++ float cast.
The minimum valid integer still returns INT_MIN without an invalid exception.

Production msg.cpp's actual function passes HONOR UBSan message-rounding-run1:
four rounding modes, nearest-even positive/negative ties, representable limits,
NaN/infinity/out-of-range sentinel and invalid exception, fractional inexact flag.
Focused build1 compiles original whole msg.cpp with unused sections discarded;
the test does not supply substitute message services. Only the test executable
was transferred; retail assets were reused. Actual snapshot/message serialization
and network operations have not executed.

All six whole normal Windows TUs pass x86/x64 compilation in
takeover_client_message_windows1. The separate Windows rounding runtime build in
message-rounding-windows1 fails linking35 broader message dependencies; no Windows
runtime parity result is claimed and failed evidence remains. Do not add fake
message services to make that fixture link. Full message owner msg_mp can supply
those actual dependencies during subsequent production integration.

Full-engine-link112 preserves signed-case compiler failure; corrected113 reaches
299 unresolved symbols in retained paths. The added full owners expose more real
dependencies; symbol-count changes are not a completion metric. Next select real
msg_mp and constant-config-string, remaining ragdoll/client/UI/memory owners, then
adapt native audio/cinematic/network dependencies. No full entry/Com_Frame result
is claimed. Previous production service/runtime probes remain evidence within
their original scope and need not be repeated absent related changes.
