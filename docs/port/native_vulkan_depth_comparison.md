# Native Vulkan depth comparison shadows

2026-10-15, stage57. Full native backend/gameplay/performance goal remains incomplete.

The native shadow target now uses real sampled D24S8 depth images and canonical depth-shadow technique2. The former R32F colour-shadow technique3 was absent from a real frontend material. This change implements the depth path rather than skipping the material or declaring unsupported services ready.

## Implementation

- Physical-device format features and image-format properties gate actual sampled/attachment/transfer usage, sample count and optional linear filtering. D24S8 comparison uses LEQUAL; nearest fallback remains available when linear filtering is unavailable.
- Shadow target image records carry an explicit comparison role. Scene-depth images remain ordinary sampled depth. Publication creates real comparison samplers; immutable draw captures derive specialization from the same image/sampler snapshot and only reflected sampler slots.
- A project-local vkd3d compile option specializes pixel SM3 sampler slots before scanning/translation. Marked samples lower to Dref with coordinate Z as reference; prior TEXLDP lowering divides both coordinates and reference by W. Comparison results replicate across all four channels. Bytecode assets remain unchanged.
- Raw and comparison variants differ in the program cache key. Native descriptor validation accepts depth-stencil-read-only layout.
- Marked bias/gradient/explicit-LOD comparison instructions, non-2D resources and vertex comparison sampling fail explicitly. This is not a claim that all future shadow shader families work.
- The canonical dummy NULL colour target becomes a genuine Vulkan depth-only pass. Shadow depth is stored because later shaders sample it; discarding its contents or making it transient would be incorrect.

## Focused tablet verification

`native-depth-stage57-tablet3.log` and `native_vulkan_shader3/manifest-depth57-3.json` record six actual Adreno610 GPU cases. A native depth-only draw writes left .25 / right .75 into D24S8. Later shaders sample that image, submit, complete a real fence and read float RGBA pixels:

| Case | Expected and observed left/right |
| --- | --- |
| nearest reference .5 | 0 / 1 |
| nearest reference .875 | 0 / 0 |
| projected coordinates/reference | 0 / 1 |
| unmarked raw depth | .25 / .75 |
| raw s0 and comparison s3 in same shader | 0 / .75 |
| linear comparison at depth boundary | .5 / .5 |

Negative stage/range/bias/cube cases reject. All six generated modules pass Vulkan1.1 SPIR-V validation. This focused mode does not rerun the historical colour/math suite or claim a full game frame.

Engine build2 passes. Checkpoint Windows token comparisons pass for r_init (15223 tokens) and r_rendertarget (4869 tokens); evidence `native-depth-stage57-legacy-proof.json`. The seven actual packaged providers match their staged SHA256 values in package verification143; engine BuildId d51fb1ffcc10d16c66bb6f544c22ea3842841d47.

## Full integration observation

SDK574/stage143, PID11872, completed actual Com_Init and reached CA_ACTIVE at166158ms. It did not report the prior missing colour-shadow technique; it terminated with a different fatal: a material requires floatZ but the readiness test checks the intentionally NULL D3D surface. Android exit-info confirms EXIT_SELF/status2, not an observer timeout or OOM. This does not yet prove a complete shadow frame or room rendering.

R_HaveNativeFloatZ now checks the actual registered R32F sampled/color image, Vulkan handles, matching owner and dimensions. All three canonical frontend/depth-prepass/backend checks use it on the native build, keeping the original failure for genuinely absent resources. D3D behavior is retained. Checkpoint legacy tokens pass for all six touched canonical sources/header in native-depth-stage57-floatz-legacy-proof.json. Five pre-existing LF lines in r_scene were normalized to the canonical CRLF encoding. Engine build4 passes.

The target sharing check now follows actual texture ownership across immutable image metadata revisions, preserving aliases during release. This is compiled but full shutdown/alias runtime validation is still pending.

SDK575/stage144 engine087329112dfcaeaf68e172723f42c8c90cbc4ce6 PID12610 reached CA_ACTIVE167854ms, then retained the same floatZ fatal. The initial observer sampled PID absence before reading the new launch log and ended early; an independent sample confirmed live12610, and follow1 continued on that same process without restart. The helper now confirms a fresh PID absence before declaring a new launch terminal.

SDK576/stage145 enginec1b844f0a8acf1b8dce9d65dcd74b07f7762c1a8 PID13399 added failure diagnostics. Read-only live memory from the matching staged ELF confirms r_floatz.current.enabled1, minimalResources0, floatZ target7 image6f15003420/1920x1200 and real nonzero sun/spot shadow dimensions. Dvar current offset0x20 and target stride0x20 were read from exact ELF DWARF, not guessed. Native-target registration details cannot be inferred from these CPU fields alone.

Actual Ninja commands revealed the new native check was excluded in r_scene and rb_depthprepass because their per-source BO1_NATIVE_VULKAN_RESOURCES flag was absent. This was an integration error in this stage. The CMake per-source list now includes both. Build5 passes; actual object undefined-symbol tables prove all three sources call R_HaveNativeFloatZ. Evidence native-depth-stage57-floatz-object-proof.json. SDK576 was deliberately force-stopped after identifying this deterministic build configuration error, rather than waiting for a known repeated failure; it was not an observation timeout or clean shutdown.

SDK577/stage146 PID14134 passes the frontend floatZ check and reaches backend model-lighting update. The depth-of-field path had a fourth independent COM-surface readiness test, which raised an error during frontend construction; rendering then also hit the already known real missing model-lighting LockBox provider at r_model_lighting.cpp821. The next stage changes that fourth floatZ check to the same actual native resource check and implements real atlas patch uploads. Full room/gameplay/performance remain unproved.

The focused and game compiler libraries have different whole-file hashes (separate build products). Their .text, .rodata, .data.rel.ro, .data and .rela.dyn sections are byte-identical; see native-depth-stage57-compiler-provenance.json. No binary identity claim is made. The compiler GPU family was not rerun for unrelated readiness/atlas changes.

Current native resource source configuration audit finds33 guarded canonical renderer sources and no absent per-source native flags; see native-depth-stage57-resource-source-config.json.

No original game assets, Windows reference files, Vulkan versions or default-backend setting changed. No asset copy, readiness stub, assertion bypass or commit was used.
