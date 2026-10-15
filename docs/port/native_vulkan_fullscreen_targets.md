# Fullscreen native targets and sharing

Stage23, 2026-10-14. Full native startup/frontend/gameplay remains incomplete.

R_InitFullscreenRenderTargetImage now attaches real native D24S8 depth storage
instead of requesting a COM depth surface when native mode is selected. Depth
images are shared by equal target dimensions. This keeps current attachment
framebuffer extents compatible; it is not a measured optimal depth lifetime graph.
Depth is sampleable/stored, not transient or lazily allocated in this path.

R_ShareRenderTarget shares immutable native color/depth records and retains the
canonical CPU target metadata. Duplicate/native initialized targets fail
explicitly. Single-target shutdown removes its native record and only releases
the registered image when no other registered target refers to it. This retains
both the image lookup owner and GPU backing for a surviving alias. Global clear
removes target records and the internally owned shared depth image registrations.
Submitted borrowers must still be retired by the renderer's submission fences.

Initial SDK514 passed attachment allocation/sharing. Review identified image
registration lifetime after source shutdown; final SDK515/stage84 includes that
fix. BuildId531a2441f11645823a6a32b1b477f2d4c8ad9ea5 passes full ARM64 link,
packaging and actual Adreno610 execution. The canonical fullscreen producer
creates64x32 RGBA16F and RGBA8 images with the same real depth record, shares the
first target, shuts its source down and verifies alias attachment/image lookup
remain valid. Final alias/other target shutdown and shared depth clearing run.
Temporary dimensions and renderer/FastFile dvars are restored after this fixture.
The established captured-material draw then remains visually correct.

These new images are allocated and ownership-checked; no new RGBA16F pass was
GPU-rendered in this fixture. Existing independent state/attachment GPU probes
still run. Full R_InitRenderTargets_PC remains blocked by unported framebuffer,
scene-null/INTZ, shadow/default-depth startup and presentation integration.
GPU-generated image readiness/layout tracking and alias sampling after a pass
remain pending. Diagnostic submission FPS is not game FPS.

Evidence: build-android-audit/native-target-stage23 build/launch logs and
legacy-proof.json, native_registry_closure/sdk-native-vulkan514/515 logs/screens.
The default preexisting r_rendertarget.cpp bodies match checkpoint4247b269 after
removing native guards. Final clean exit evidence is SDK515-exit.log.

Next native framebuffer/scene startup and material submission integration; do
not turn a partial target initialization into successful native Com_Init.
