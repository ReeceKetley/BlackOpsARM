# Real native D3DX compiler adapter

2026-10-13. Full-engine-link125 compiles the real compiler adapter and has no
remaining D3DX unresolved symbols. It still fails linking broader engine services;
actual Com_Init/Com_Frame remain incomplete.

Full engine selects pinned upstream CPU compiler as an independent shared target.
Original D3DXCompileShader now invokes actual HLSL-to-D3D_BYTECODE compilation,
copies bytecode/diagnostics into native COM buffers and obtains owned CTAB via the
bounded metadata parser. Compiler-owned output/messages are always released;
shader/table outputs publish only after all requested objects succeed. Failures
leave shader/constants null. No fake shader/empty constant success was added.

Native ID3DXInclude now declares SDK's actual two callback slots. Adapter maps
local/system type and parent data, preserves returned byte ranges until Close and
propagates callback failure. Macros are passed to the real preprocessor. Without
an include handler, includes fail explicitly instead of implicitly opening POSIX
files. The original loader normally expands its file includes before this call.

Current consumed flags supported: SKIPVALIDATION, default/row/column matrix packing,
backward compatibility, legacy D3DX DLL selection. Legacy selection is replaced
by this actual native compiler backend, not emulation of a Windows DLL or a claim
of identical optimizer output. SKIPVALIDATION permits rather than mandates skipping
the backend's validation; native validation remains. Packing/backward options map
to genuine upstream settings. Unsupported debug/optimization/precision/flow options
and conflicting packing requests return E_NOTIMPL/error diagnostics. No silent
support claim for those flags. Native compiler still has its own feature limits.

Windows x64 SDK fixture d3dx-compile-windows1 compiles the actual loader flags
2|65536 with default/row/column float4x3 packing: class3/2/3, rows4 columns3,
register counts3/4/3. It also passes predefined macro plus nested local/system
include callbacks, exact parent identity, balanced opens/closes, syntax error
diagnostics/null outputs and table lifetime after shader release.

HONOR adapter UBSan build2/run1 passes the same common assertions and identical
printed metadata, compared in parity1, plus missing include-handler and unsupported
option failures. This uses production adapter/CTAB/buffer sources. Upstream compiler
shared library itself is not instrumented by this probe. Full engine builds its
compiler Debug; standalone predecessor was Release, so the actual full-build
library was transferred and tested again in production-library-run1, also passing.
Actual full compiler SHA256:
cd4f317e3aa85ea3f053a14cbf2f1696565db844173519e48fd2124060d515eb.
Only changed library/test binary were pushed; staged game assets were reused.

Initial124 compiler errors from the Windows interface macro and an incorrect
upstream enum spelling were repaired, preserved in its log. Probe build1 had an
unquoted PowerShell include argument split at vkd3d-1.17, corrected in build2.
These are corrected failures, not passing evidence. Source/vendor licensing,
grammar generation and dependency manifests remain intact.

This does not prove byte-identical shader output, all retail shaders, full SDK
constant-setting interfaces, actual material loading or GPU rendering. The
original material diagnostic line mapper expects Windows formatting; native
compiler messages currently use upstream formatting and may reach its existing
"Message format changed" fallback. Error text is preserved; source-line mapping
needs adaptation before claiming complete original diagnostic integration.
The original caller's cache/error/runtime behavior has not been executed.

Next resume the remaining real client/UI/memory/script/audio/cinematic/game/
renderer/network owners from link125, then execute actual startup. Existing shader
probes need not be repeated absent related changes. Preserve pinned Vulkan/DXVK
requirements and dynamic CPU library/source/license packaging.
