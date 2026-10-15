# Native framebuffer and scene initialization

Stage24, 2026-10-14. Native Com_Init/frontend/gameplay remains incomplete.

R_InitFrameBufferRenderTarget now creates real native offscreen BGRA8 framebuffer
storage through the canonical fullscreen image allocator. Scene aliases that
attachment record; non-minimal initialization creates a separate resolved-scene
color image. Matching sizes share native depth storage. The reported D3D format
is checked against the actual Vulkan image format, rather than fabricated from a
COM surface description. D3D9 remains unchanged in its default branch.

This offscreen image still needs explicit engine submission/resolve and final
window presentation integration. The function does not claim that presentation
has happened. Reflection-probe framebuffer capture fails explicitly pending its
native integration. Global target initialization still has unported INTZ,
scene-null/shadow secondary attachment and other startup operations.

SDK516/stage85 BuildIdd1aaf31a2d528071738d848f5f3a11565082f6c6 passes ARM64 link,
packaging and actual authorized Adreno610 execution. A64x32 fixture drives the
canonical framebuffer initializer, checks scene alias and separate resolved
color ownership, creates a real VkRenderPass/VkFramebuffer from those views,
releases the source while verifying the scene image remains registered, and
shuts all target owners down. Dimensions, minimal/reflection options and the
fixture renderer/FastFile dvars are restored. The new framebuffer was created,
not GPU-drawn or presented as a live scene in this check. The existing captured
material/state GPU checks continue passing and the diagnostic remains visible.

Initial build used the wrong AttachmentPass constructor arity; it was corrected
before packaging. Preserve native-frame-stage24-build1.log as failed evidence.
The successful launch/build log is native-frame-stage24-launch1.log. Additional
evidence: native-frame-stage24-legacy-proof.json and native_registry_closure/
sdk-native-vulkan516 presentation/exit logs and screenshot. Removing only native
guards reproduces the preexisting r_rendertarget.cpp Windows bodies at4247b269.

Next depth-only/global target startup, GPU-generated image layout/readiness,
production material submission and framebuffer-to-window presentation. Do not
return successful native hardware/Com_Init after only these target allocations.
