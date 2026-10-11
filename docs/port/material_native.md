# Native material loader layouts

## Batch56: technique, argument and state storage

Loose material loading still contained Win32 allocation sizes, pointer strides
and copies. Technique sets now allocate `sizeof(MaterialTechniqueSet)` plus
their owned name. State maps use `sizeof(MaterialStateMap)` plus their name.
Techniques allocate the native header, one to four native passes, full-width
trailing state-map pointers and their name. Named fields replace byte offsets.

Pass arguments allocate and copy native `MaterialShaderArgument` records.
Argument appending and compaction preserve the entire union, including literal
pointers above4GB. Literal setters write `u.literalConst`. Two scalar locals
left by the decompilation are restored to their original scratch capacities:
512 arguments and130*4 pairs of state bits. State remapping reads trailing
state-map pointers with their native stride. Remap rules, sorting, frequency
selection, merging and deduplication keep their existing logic.

## Focused evidence

`tools/port/check_material_native.ps1` extracts the actual production types,
frequency tables and19 functions. Seven separate process cases pass on both
Win32 and unrestricted x64. Allocated x64 objects are guarded and above4GB.
The cases cover one to four passes, technique slots0/129, all ten state-map
rule pointers, vertex/pixel literals, matrix-row argument merging, full native
argument copies/appends,512 scratch slots and130 technique state selections
with overlapping four-pass sequences deduplicated to six state pairs.

File contents/tokenization, shader reflection/linkage and declaration loading
are controlled helpers. The512-argument case is a memory-capacity check; it
does not represent a valid512-sampler GPU shader. State flag callbacks record
invocations. These tests do not certify the complete loose-file loader or COM
shader compilation.

With `-NegativeControl`, the pinned1fca062 implementation fails all seven x64
cases and Win32 parser/state cases; its other five Win32 cases pass. Failures
include pointer truncation, native allocation overruns and scratch corruption.
The old implementation is extracted into ignored test fixtures; the production
file is not replaced. Results are retained in material56_cases.json and
material56_baseline_cases.json. `material_report.py` gathers focused logs,
source hashes, build logs and fresh runtime gates into ignored build/run reports.

All five configurations compile with zero errors: Win32, low-address x64,
unrestricted x64, low-address guard and unrestricted guard. The material file
has six remaining truncation warnings on x64, down from ten in its previous
compilation. The latest full warning audit remains144 sites from batch47;
this incremental build does not establish a new whole-project count.

## Runtime gates

Final56 original Win32/high/low/guard routes 25/25/32/26PASS, all0FAIL/exit0/all4Savechecks. Windows Vulkan56 26PASS/0FAIL/exit0/all4Savechecks with pane2, actual upgraded weapon, 30000ms continued play and zero validation errors through teardown. All test games ended.

The patched local DXVK/system Vulkan/NVIDIA modules and explicit layers are
recorded. Sound is enabled and automation uses god mode. An actual presented
frame is preserved and inspected: normal rendered gameplay in the elevator.
Packaged gameplay is separate from loose-file loader coverage. Reports retain
frozen executable hashes and full gate lines.

## Remaining scope at batch56 (superseded below) at batch56 (superseded below)

WARN0014 tracks the rest of this pipeline: typed D3DX compiler outputs and COM
calls, native vertex/pixel shader header allocations, the vertex bytecode copy
length, failure-path buffer releases, raw material/texture creation and full
file loading. Normal packaged gameplay does not verify those paths. WARN0013
separately tracks actual simulation-water material/debug rendering. Android
will need replacements for Windows D3DX and platform services; this milestone
continues to target Windows Vulkan.

At batch56 the next verification was a30-minute sound-on session using this
frozen unrestricted x64 snapshot. Short routes cannot establish late audio,
script lifecycle or memory stability. TASK0021 records that follow-up; the
loose-shader pipeline audit continues afterward.

## Native shader compiler and ownership (batch57)

- Batch57 (FACT0100/0101/0102/0103): typed D3DX diagnostics/constants and COM ownership, native shader headers, proper vertex bytecode copy and CreateShader failure buffer releases fixed. Real D3DX guarded tests15cases per host pass, x64 COM/objects above4GB; pinned controls fail12Win32/13x64 cases. Controlled generation/cache/device/failure helpers; warning is a controlled real COM buffer, invalid HLSL yields real diagnostics. All5builds0errors/no material truncation warnings. Full high426s/0errors/134sites (132src/tl+2DemonWare), down144; gfx_d3d21 remain. Final57 Win32/high/low/guard 24/32/25/25PASS, all0FAIL/exit0/all4Savechecks. Actual GPU shader checks pass Win32D3D9/x64D3D9/x64DXVK with expected pixels and clean Vulkan teardown; final57 Vulkan 28/0/exit0/30seconds continued/clean teardown. Frozen56 Vulkan soak passes 1800000game ms after actual PaP, max round16/520spawned,0FAIL/exit0/clean teardown. All test games ended. WARN0016 replacesWARN0015 for remaining full loose-file/raw-material/reflection/cache integration. WARN0013 water material/debug remains. Windows platform docs remove old Godot bridges and preserve current native DXVK direction. Followups49-57 form the verified checkpoint; publication SHA is recorded in ledger. Reports shader57_results.json/shader57_focused_results.json/vulkan56_soak_results.json;56 soak executable predates57. Initial DEVICELOST helper attempts remain preserved; the same device setup later passed, transient cause unconfirmed. GPU manifests hash actual loaded modules, including system x64 D3DX (which differs from the staged DLL) and WOW64 system paths.
