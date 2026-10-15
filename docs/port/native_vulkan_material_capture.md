# Native material selection and draw capture — stage 15

2026-10-14. Full goal incomplete. SDK504/stage73 BuildIdf3eaa878bab19a9f30bf146d9919a2bbf786b1d3 installed. Uncommitted after pre-native checkpoint.

material_bindings owns immutable selected VS/PS/declaration records per canonical command state. Canonical R_SetPixelShader, R_SetVertexShader and R_UpdateVertexDecl now select actual native records/available variants without fabricated COM handles. Native command init/local copy release/clone selection. Registry shader/declaration unload leaves already selected/captured records valid.

Capture combines selected shaders/declaration, constants, textures, geometry and effective render-state words into one owned draw packet. It must run on the command state's owning renderer thread without concurrent writers; module lookups are synchronized individually, not one global atomic transaction. Missing required selection/constant/geometry state fails explicitly. The recording scope now consumes captured geometry instead of querying later live geometry state.

Upload allocates fresh aligned VS/PS float-bank ranges from a caller-owned uniform arena, validates capacity/overflow and reflected resources, then copies immutable bytes and returns matching descriptor resources. Caller resets cursor only after submitted readers retire and must reserve capacity in advance. Current integer/boolean banks reject explicitly; separate vertex texture banks and GPU-generated image readiness still unfinished. The API is per-draw range allocation, not a completed production descriptor/PSO cache.

Preview calls actual shader selectors/declaration update before owner unload. Main pipeline is created from selected native records. Live texture binding remains until command teardown; disable/unload is tested on a copied command state. Captured banks uploaded once through new Upload (removed redundant manual writes); main draw retains its packet/geometry in fence-controlled frame slots. Production PSO lookup on each changed selection/state/layout, actual complete material argument setup and all draw-family integration still need work.

SDK504 actual T5 room material capture renders upright/expected colors from independent uniform range. A later captured CPU state mutates register255, uploads atPSoffset12288, and verifies earlier mappedPSoffset4096 and its snapshot remain unchanged. First captured range is GPU-consumed; later range is host-checked, not independently shader-sampled. SDK503 intermediate5527submissions cleanexit retained. FinalSDK5042035submissions BACK exit0/resources_released0 USB0. Existing semantic GPU checks pass. No nativeCom_Init/live scene/gameplay/FPS proof. Two complete material packets on a production scene are not established by this fixture.

ARM64 target links with assertions/UBSan/debug settings preserved. Guard inverseproof exact preexisting r_shade/r_state_utils/rb_draw3d bodies after newline normalization. Original assets reused, Windows/DXVK read-only. No changes to default D3D9 selection.

Next: native hardware/global sampler/target setup toward actual Com_Init, dynamic/world/queued geometry and packed vertex formats, complete material argument/canonical command submission, state scope/shutdown and per-draw PSO/descriptor caches. Native scene/gameplay/mobile timings remain required; BC memory/streaming and optional policies still incomplete.

Evidence: build-android-audit/native-material-stage15-build.log, native-material-stage15-device.log / device504.log, native-material-stage15-legacy-proof.json; native_registry_closure/sdk-native-vulkan503/504-screen.png and exit.log; apk-native-stage73/native-manifest.json/package verification.
