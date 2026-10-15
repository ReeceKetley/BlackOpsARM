# Native depth-only scene targets and pipelines

Stage25, 2026-10-14. Full native Com_Init/frontend/gameplay remains incomplete.

Native R_InitSceneNullRenderTarget_PC now creates a real depth-only attachment
record sharing the scene depth image, with no dummy color surface or COM handle.
It respects the existing allow-null-target option and requires an actual source
depth owner. The target retains canonical dimensions. Pipeline descriptions now
select zero or one color attachment; the default remains one. The pixel shader
is retained so discard/alpha behavior remains available in depth-only passes.
Production material dispatch must select this field from its actual pass.

SDK517/stage86 BuildId0e88c0ec1ad1c05e08a170af32ad2088716214bb passes ARM64 link,
packaging and actual Adreno610 execution. The original scene-null initializer
creates a shared D24S8 depth record and a real depth-only framebuffer in the
bounded scene startup fixture. A separate GPU probe creates a zero-color-attachment
pipeline with the actual T5 vertex shader and fixture pixel shader, clears D16
depth to0.5, draws a triangle with LESS/depth-write enabled and reads back0 after
the submission fence retires. This proves actual depth-only GPU writing without
a color attachment. Existing alpha/depth/blend/stencil/cull probes still pass.
Depth-only alpha discard is not independently exercised by this new probe.

The scene startup fixture temporarily/restores allow-null-target configuration.
New scene targets are not live frontend frames; diagnostic FPS is not game FPS.
Depth is retained/sampleable and stored in this path, not transient. Later pass
lifetime optimization must establish actual consumers before discarding it.

Evidence: build-android-audit/native-depthonly-stage25 build/launch logs and
legacy-proof.json, native_registry_closure/sdk-native-vulkan517 logs/screenshot.
Preexisting r_rendertarget.cpp Windows bodies match checkpoint4247b269 after
removing only native guards. Clean exit evidence is517-exit.log.

Next INTZ/fallback and shadow/global target startup, generated image readiness/
layouts, production material dispatch and framebuffer-to-window presentation.
Hardware/Com_Init must not return success for a partial startup sequence.
