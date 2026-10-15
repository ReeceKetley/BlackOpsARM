# Native dynamic vertex and index buffers

Stage17, 2026-10-14. This implements real startup allocations and upload paths
required by the native backend. Full native engine startup remains incomplete.

`R_InitDynamicVertexBufferState` and `R_InitDynamicIndexBufferState` reserve real
Vulkan buffers keyed by the canonical COM owner slot, leaving the COM pointer
null. Vertex capacity/used are bytes; index capacity/used are 16-bit indices.
`R_SetVertexData` publishes uploaded bytes and returns the original byte offset.
`R_SetIndexData` retains the canonical reservation/wrap behavior, publishes the
index bytes and binds the actual slot through `R_ChangeIndicesFromOwner`.
Dynamic pool shutdown removes native owners before the existing CPU teardown.
Water simulation and other shutdown paths still need their own native adapters.

Dynamic storage requires host-visible coherent memory and prefers device-local
memory where compatible. Appends are limited to unused bytes. Offset zero is a
discard: retained record or GPU backing ownership forces a new allocation;
otherwise the existing backing can be reused. Captured records keep immutable
payload lengths and prior backings alive. Expired generations are pruned while
tracking new allocations, so the tracker does not grow with every frame.
No per-upload staging submission or device idle is introduced. A device without
compatible coherent memory fails explicitly; a noncoherent fallback is pending.

SDK507/stage76 tested the initial implementation on the authorized Adreno610.
The canonical producer test used a two-triangle-capacity vertex/index ring,
checked first/append offsets, shared append backing, new discard backing,
retained bytes, overlap rejection, and rendered the captured material using the
final dynamic records after removing owner slots. Earlier records were retained
by the CPU during discard; this does not independently prove an overlapping
in-flight GPU read of the earlier generation. Existing command/frame ownership
retains geometry records until their submission fence retires.

The subsequent SDK508/stage77 includes bounded retired-generation tracking.
Raw launch, presentation, exit logs and screenshots are under
`build-android-audit/native_registry_closure/sdk-native-vulkan507-*` and
`sdk-native-vulkan508-*`; build/launch logs are
`build-android-audit/native-dynamic-stage17-*`. Final runtime details are in STATE.
The preexisting Windows bodies of r_buffers.cpp and r_shade.cpp match checkpoint
4247b269 after removing only the native guards; proof is
`build-android-audit/native-dynamic-stage17-legacy-proof.json`.

The native diagnostic is still a captured-material draw with Com_Init_called=0.
It is not live frontend geometry or a game FPS benchmark. Next integrate the
owner-aware dynamic stream consumers in tess/skinned/static-model draws and
finish queued/world/render-target startup before authentic native Com_Init.
