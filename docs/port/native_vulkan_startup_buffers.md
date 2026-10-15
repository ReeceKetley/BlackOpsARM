# Native startup buffer allocation and completion

Stage19, 2026-10-14. Full native Vulkan engine initialization is incomplete.

The low-level dynamic vertex/index allocators now reserve real coherent mapped
Vulkan buffers by canonical owner slot. Their returned null pointer means the
allocation is unlocked, matching the original contract; allocation failure is
fatal and never presented as readiness. COM pointer slots remain null.

Static allocators return a real mapped Vulkan memory pointer for the canonical
producer to fill. An unpublished owner map retains the allocation while filling.
Acquire cannot see that record until matching typed owner-aware Finish completes.
Pending owners prevent backend/device detachment and can be explicitly released.
Coherent host-visible memory is required; device-local memory is preferred.
The returned mapping belongs only to initialization and must not be written after
Finish/publication. A noncoherent initialization fallback remains pending.

Owner-aware finish wrappers delegate to the original COM unlock on D3D9. Native
particle-cloud generation, water index generation, queued index uploads and world
vertex generation now finish through their actual owner slots. Water and particle
shutdown remove native owners even though their COM slots remain null. Published
borrowers retain their Vulkan backing after owner destruction.

SDK510/stage79 BuildId6b338aeb464482b044f282bd53905868fb89fcc3 passes the full ARM64
engine link and actual authorized Adreno610 APK execution. The diagnostic calls
the original R_InitWaterSimulationVertexBuffers/R_FreeWaterSimulationVertexBuffers
and R_CreateParticleCloudBuffer/R_DestroyParticleCloudBuffer. Real tile indices
and original particle generation execute, with published particle capacities
65536 vertex bytes and24576 index bytes and retained records after owner release.
It then renders the previously established material. These startup builders ran;
queued/world producers are compiled but their native production loading execution
is not yet proved. No native Com_Init or live game frame is claimed.

Evidence: build-android-audit/native-startup-buffers-stage19-build1.log,
native-startup-buffers-stage19-launch1.log, native-startup-buffers-stage19-legacy-proof.json,
and native_registry_closure/sdk-native-vulkan510 presentation/exit logs and screen.
The body proof reverses only owner-aware finish calls to their exact original
default calls and removes native guards/new wrappers; preexisting Windows bodies
match checkpoint4247b269.

Remaining work includes mesh/skinned/water/cache upload and retirement consumers,
remaining queued shader/resource actions, world unload, global render-target
initialization/sharing/resolve, native material pipeline dispatch, device/window
startup and actual native Com_Init/frontend/gameplay.
