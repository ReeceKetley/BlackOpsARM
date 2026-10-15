# Native static-model cache patches

Stage21, 2026-10-14. Full native Com_Init, live frontend and gameplay remain unproved.

Dynamic buffer UpdateRange supports bounded partial patches without discarding
unrelated published data. An overlapping patch creates new backing when the old
record/GPU buffer is borrowed, copying the published prefix before updating it.
Non-overlapping writes use unused coherent memory. Gaps are zeroed before the
larger payload is published. Range/alignment and pending-map checks fail explicitly.
Copying an old prefix is CPU work; this is a functional implementation, not a
measured cache-performance improvement or GPU transfer optimization.

RB_PatchStaticModelCache routes canonical staged vertices and offsets through
UpdateRange, with a bounded staged-vertex check. Cached model stream callers bind
the cache owner and retain the original offset calculation. Ordinary rigid static
model vertex/index helpers now use owner-aware native binding. Cache shutdown
performs the original CPU flush and removes the native GPU owner.

SDK512/stage81 BuildId30dc2ba53ccee248c3c53a1c68a236710856f9c8 passes ARM64 linking,
packaging, actual Adreno610 execution and the legacy body proof. The original
R_InitStaticModelCache creates its real8388608-byte buffer and CPU cache lists.
Two bounded partial patches verify old borrower bytes, copied unrelated bytes,
initial zero gap and capacity rejection. The original cache shutdown runs.
Rigid static binding helpers execute against canonical XSurface records.
The subsequent captured-material draw remains visually correct and cleanup is
recorded in the exit log. Old patch borrowers are CPU-held in this check;
independent overlapping GPU reads and production scene-driven cache patching
are not separately proved. Diagnostic submission FPS is not game FPS.

Evidence: build-android-audit/native-cache-stage21-build1.log,
native-cache-stage21-launch1.log, native-cache-stage21-legacy-proof.json and
native_registry_closure/sdk-native-vulkan512 presentation/exit logs and screen.
After removing only native guards, preexisting rb_backend/r_draw_staticmodel/
r_staticmodelcache Windows bodies match checkpoint4247b269.

Next water tile updates, remaining queued resource/world release paths and
render-target/material/device-window integration, then genuine native Com_Init.
