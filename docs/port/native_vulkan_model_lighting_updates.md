# Native Vulkan model-lighting atlas updates

2026-10-15 stage58. Goal remains incomplete.

SDK577 reached a genuine null D3D volume-texture LockBox call in RB_PatchModelLighting after CA_ACTIVE. Its separate depth-of-field floatZ error still used a COM-surface test. The fourth readiness site now uses R_HaveNativeFloatZ, with actual Vulkan resource checks and the existing failure for missing resources.

Model-lighting allocation already created the actual 256x256x4 BGRA8 Vulkan image. A native CPU texel backing store now accompanies that exact owner/texture generation. It starts at zero and queues an actual immutable upload; this is data initialization, not successful-rendering readiness. The existing canonical patch loop writes into the backing using RowPitch1024 and SlicePitch1024*height. It preserves all other texels between patch batches. Patch indices must remain inside the actual atlas; the update rejects a stale image generation or recording frame.

At unlock, QueueUpload snapshots the CPU pixels into the production image upload queue. The canonical backend calls RB_PatchModelLighting before EngineFrames::Begin, which already flushes actual staging copies, transitions and fences before recording scene reads. No fake IDirect3DVolumeTexture is introduced. CPU state resets at atlas shutdown; GPU snapshots retain their own lifetimes.

This correctness baseline uploads the full1MiB atlas per changed patch batch (2MiB if the configured height is512). It is not a measured mobile performance win. Region uploads and retained CPU snapshot memory remain optimization candidates after visible rendering works.

Engine build1 passes. Windows checkpoint legacy bodies compare equal: r_scene43579 tokens and r_model_lighting5605, explicitCP1252/CRLF. Three pre-existing LF lines in model-lighting source were normalized to CRLF. Evidence native-atlas-stage58-legacy-proof.json. No historical shader/math families were rerun for this integration change.

SDK578/stage147 engine51a71ef82613cc0c0a542f21757e6e261f090443 PID14891 reached CA_ACTIVE167389ms. The previous DoF floatZ error and model-lighting LockBox UBSan did not recur. The canonical backend completed RB_PatchModelLighting and subsequent EngineFrames::Begin upload flush; PID-filtered log at08:04:13.958 records one1MiB image/subresource uploaded with actual completed fence. The next real error is Native clear requests an absent attachment aspect, in the shadow draw callback. The process then exited; no timeout restart was used.

This proves execution through the native model-lighting patch and upload, not correct lighting pixels in a visible room. Full atlas visual correctness, room/gameplay, GPU timing and performance remain unproved. Stage59 is adapting the shadow callback dummy-colour clear plus actual native scissor/depth-range controls. See current STATE for latest result.
