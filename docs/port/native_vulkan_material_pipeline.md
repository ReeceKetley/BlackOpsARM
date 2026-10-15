# Native Vulkan material pipeline — stage 2

2026-10-14. Native replacement goal remains incomplete. Builds on stage1 shader translation; introduces reusable production pipeline/descriptor code in `src/gfx_vulkan/graphics_program.{h,cpp}`. No game backend switch or performance gain claimed.

GraphicsProgram owns immutable Vulkan pipeline/layout objects and translated shader interfaces, accepting the caller's device, render pass/subpass and pipeline cache. It supports vertex layout/topology, sample count, culling/front face, depth comparison/writes and a color blend attachment, with dynamic viewport/scissor. Shader pairs are semantically linked. Resources bind through reflected VS0/PS1 constant, texture and sampler slots. Missing/duplicate resources and invalid handles/layouts fail explicitly before descriptor allocation. Descriptor pools belong to fence-retired frame arenas; this code does not reset pools or take ownership of input buffers/images. Caller must retire GPU work before destruction. This is a material pipeline primitive, not complete handling of all T5 state, images or frame scheduling.

The shared tablet draw harness now exercises this production path for both the LRP fixture and the real untouched T5 fullscreen VS/colour PS. Actual material draw uses aligned uniform buffers, an optimal-tiled sampled texture, separate image/sampler descriptors, upload/layout transitions, dynamic viewport, draw, fenced completion and RGBA32F readback. Native fullscreen constants use pixel-centred Vulkan placement and a negative viewport height; no claim of matching every legacy D3D half-pixel convention. General engine coordinate adaptation remains part of integration.

## Device results

- Adreno610/Vulkan1.1.128, Debug/UBSan-no-recovery wrapper/harness.
- Initial neutral actual-shader gradient draw:2,560pixels, maximum error0.
- Subsequent actual-shader draw uses a64x40 sample of the previously captured real room image and red scale0.75 in all three colour matrix banks. All2,560RGBA pixels match the independent expected scaled input, maximum float error0. This deliberate change checks that uniforms affect the rendered output, rather than merely presenting an unchanged texture.
- Result PAM converted to PNG and visually inspected: room geometry/image orientation retained, deliberate colour adjustment present. This low-resolution fixture proves resource/shader integration, not final game image quality.
- Real shader outputs retain Vulkan1.1 SPIR-V validation. LRP extrapolation check still passes through GraphicsProgram.
- GamePID30744 unchanged; no APK/provider replacement, no game asset transfer/import. Only changed native harness binaries and a10,240byte captured-image fixture transferred.

Evidence: `build-android-audit/native_vulkan_shader3/build.log`, `tablet-room-tint.log`, `manifest-room-tint.json`, `room64x40.rgba`, `t5-colour-room-tint.pam/png` and exported SPIR-V/disassembly/validation. Earlier neutral run preserved as `tablet-final.log`/`manifest.json`. Reproduce changed-colour fixture with `run_native_vulkan_shader_smoke.py --build build-android-audit/native_vulkan_shader3 --remote /data/local/tmp/boarm-vulkan-shader3 --label room-tint --image build-android-audit/native_vulkan_shader3/room64x40.rgba`; use a fresh label to preserve earlier logs. The input is derived from `docs/port/evidence/native_post_fusion_device478/bloom-fusion-control-223725537057883-original.pam`, sampling source pixel centres. No full-game frames produced by this native path yet.

Next: native SDL surface/swapchain/frame ownership and presentation, followed by engine resource/provider integration. Remaining full scope includes all material states (alpha test, stencil, shader parameters, MRT/depth-only variants), native image/buffer lifetimes, frontend geometry/lighting/postprocessing, gameplay/surface recovery and measured mobile optimizations. Runtime still uses saved D3D9 reference; checkpoint4247b269 unchanged.
