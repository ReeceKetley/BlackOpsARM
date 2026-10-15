# Native auxiliary vertex stream and world unload

Stage22, 2026-10-14. Full native startup/frontend/gameplay remains incomplete.

Auxiliary vertex stream initialization verifies actual native allocation, clears
real mapped memory and records its capacity. Lock preserves the existing published
bytes, replacing backing if an earlier borrower remains. Unlock publishes the
completed mapping. Missing/pending owners fail explicitly. Free and world unload
remove native slot owners while earlier records keep their GPU buffers alive.
World unload preserves the original CPU lighting/cache reset and auxiliary stream
shutdown ordering. The default D3D9 bodies and assertions remain intact.

SDK513/stage82 BuildId8b2e8391bac871a2ea6e3a469a3cb2b6cd09ca08 passes ARM64 linking,
packaging and the authorized Adreno610 run. A128-byte stream is filled then
partially updated through its original lock/unlock API; readback verifies both
the earlier borrow and untouched current bytes. The canonical world-buffer
producer creates a44-byte vertex fixture and12-byte layer fixture. R_ReleaseWorld
removes their owners and the auxiliary stream owner; borrowed buffers survive.
This uses temporary/restored renderer-load dvar and world count fields solely to
drive the bounded fixture. It is not original map loading or live world drawing.
The subsequent captured-material render remains visually correct.

Shader queue audit: canonical FastFile shader producers in
r_material_shader_load.inl already call typed shader_assets::Create when native
mode is active. Raw RB_Resource_CreateVertexShader/PixelShader currently have no
source call sites beyond their definitions. Generic queued resource handling and
loose material/cinematic shader paths still need integration where exercised;
this audit does not claim those paths are complete.

Evidence: build-android-audit/native-world-stage22-build1.log,
native-world-stage22-launch1.log, native-world-stage22-legacy-proof.json and
native_registry_closure/sdk-native-vulkan513 presentation/exit logs and screen.
After removing native guards, the preexisting r_bsp.cpp and r_vertexstream2.cpp
Windows bodies match checkpoint4247b269. Prior borrowers are CPU-held in this
check; overlapping old-generation GPU reads are not independently proved.

Next global render-target/device-window initialization and production material
submission integration; water tile updates and remaining multistream/world draw
consumers remain pending. Native Com_Init and gameplay have not run.
