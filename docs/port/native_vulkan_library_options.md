# bgfx and Sokol — focused fit check

2026-10-14. User suggested bgfx or "Soko"; Sokol is inferred, not confirmed. Read-only upstream check, no new dependency adopted and native replacement goal unchanged.

**bgfx:** a cross-platform rendering abstraction with Vulkan and Android support. Current Vulkan source checks for Vulkan1.1 or newer, so the tablet's version is not an immediate exclusion, although full capability compatibility remains untested. It owns resource/pipeline caches, draw bucketing and a render thread. Its sequential view mode preserves submission order when required. Integrating T5 still requires resource/material/state adaptation and shader conversion: its Vulkan ShaderVK loader expects bgfx binary metadata, not arbitrary retail SM3 bytecode or simply a raw SPIR-V module. Adopting it would trade low-level implementation effort for adapting to its resource, shader and scheduling contracts. Could be worthwhile, but no measured gain or faster migration demonstrated.

**Sokol:** smaller C rendering wrappers. Current SOKOL_VULKAN gfx source uses dynamic rendering, synchronization2 and descriptor buffers; setup treats missing required extension functions as fatal. Author's Vulkan backend introduction specifies Vulkan1.3 plus EXT_descriptor_buffer and bufferDeviceAddress, with mobile limitations. This is a poor fit for the pinned Adreno610/Vulkan1.1.128 target without a significant alternative backend/fork. Do not confuse Sokol's GLES Android support with suitability of its current Vulkan path.

Recommendation/inference: study bgfx caching, submission and lifecycle design, continue the engine-owned Vulkan1.1 implementation for now. Keep evaluating whether a library reduces total integration work, rather than assuming another abstraction automatically provides mobile pass-lifetime knowledge. Shader/resource/pass dependencies still belong to T5; new library adoption requires a concrete integration advantage.

Primary sources checked:

- [bgfx repository/platforms](https://github.com/bkaradzic/bgfx)
- [bgfx internal ordering/thread model](https://bkaradzic.github.io/bgfx/internals.html)
- [current bgfx Vulkan source](https://github.com/bkaradzic/bgfx/blob/master/src/renderer_vk.cpp): version checks, cached render passes, ShaderVK binary reader
- [current Sokol gfx source](https://github.com/floooh/sokol/blob/master/sokol_gfx.h): vkCmdBeginRendering, vkCmdPipelineBarrier2, required vkCmdBindDescriptorBuffersEXT
- [Sokol author's Vulkan introduction](https://floooh.github.io/2025/12/01/sokol-vulkan-backend-1.html): documented requirements of original implementation

Upstream master is moving; pin and recheck exact revisions/capabilities before adopting either. No whole project checkout/build or game restart was necessary for this comparison.
