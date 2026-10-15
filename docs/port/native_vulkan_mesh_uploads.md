# Native mesh and backend uploads

Stage20, 2026-10-14. Native engine startup and live gameplay remain incomplete.

`R_BeginMeshVerts` maps real coherent Vulkan storage through a dynamic owner.
Mapped discard reuses storage only when no prior record/GPU borrower remains;
otherwise it allocates a replacement. The owner cannot be acquired or uploaded
while mapped. `R_EndMeshVerts` publishes the completed record and clears the
producer pointer. This preserves the canonical direct-fill mesh flow without
inventing a COM handle or adding a staging submission. Dynamic mesh shutdown
removes the native buffer owner before the existing CPU index cleanup.

`RB_UpdateDynamicBuffers` now uploads the actual staged skin, mark, code, glass
and pre-tess index bytes to their native owners. Mark/code/glass and pre-tess
retain the original capacity clamp; the skin range must fit its allocation.
The staging-frame flag is released only after successful uploads. It represents
completion of CPU staging reads, not GPU completion. Command/frame records must
continue retaining GPU backings until their actual submission fences retire.

SDK511/stage80, BuildId5d84c3b508452b354f6c9ec6b9a3405ff703616b, passed ARM64 link,
packaging and the authorized Adreno610 run. The original R_InitDynamicMesh and
R_SetQuadMeshData create a real128-byte quad; a second update preserves an earlier
borrowed buffer's contents. The actual bulk update function executes against a
bounded staged-data fixture for all five buffer families. A second skin upload
uses new backing while the old record retains its original bytes. Staging-frame
release is checked. These are CPU-held borrowers; independent overlapping GPU
reads of those earlier generations are not separately claimed.

The diagnostic subsequently renders the established captured material. Full
production skinning/scene/render-thread execution through native Com_Init is
unproved, and diagnostic FPS is not game FPS.

Evidence: build-android-audit/native-mesh-stage20-build1.log,
native-mesh-stage20-launch1.log, native-mesh-stage20-legacy-proof.json and
native_registry_closure/sdk-native-vulkan511 presentation/exit logs and screen.
After removing only native guards, the preexisting Windows bodies in
r_meshdata.cpp, r_rendercmds.cpp and rb_backend.cpp match checkpoint4247b269.

Next finish static-model cache/water update and retirement, remaining queued
resource actions/world unload, global render-target setup and native material
pipeline dispatch, then connect actual device/window startup and Com_Init.
