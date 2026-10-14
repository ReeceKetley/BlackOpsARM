# Original client pose and debug source integration

2026-10-13. Full Com_Init remains incomplete; no new tablet runtime claim.

Selected original cg_drawtools, cg_pose_mp, cg_ents_mp, renderer debug/debug
allocation, client skeleton allocation, pose utilities and model-skin SSE
translation units. These supply material-handle, primary-light, pose transform,
debug shape and pose-state dependencies from the actual canonical owners.

The x64/ARM branch of r_model_skin_sse already calls the real scalar skinning
algorithm. Selecting it supplies functional reference behavior, not an empty
SSE endpoint or simulated hardware support. ARM machine-info still reports
SSE=false. It does not claim NEON acceleration or measured rendering performance.

Canonical five system-info Dvar globals and Sys_RegisterInfoDvars moved outside
win_main's selected headless guard. The original registration code and flags
are unchanged and read real sys_info. This makes its actual owner available;
it does not register early or replace Sys_Init. Sys_Init's existing diagnostic
endpoint still needs real integration before later startup/frame execution.

Android entity prefetch uses the native compiler operation; Windows keeps its
intrinsic. The optional mover log uses engine writable user root and existing
native stdio service. Quaternion-to-axis writes now index real matrix rows,
preserving values while removing array-subobject bounds violations.

## Evidence

Native source compilation succeeds; full-engine-link64 fails on actual remaining
dynamic-entity, collision/mapkit, cinematic/sound, client state and recovery
dependencies. Logs61..63 preserve prior source integration/compiler failures.
Normal Windows x86/x64 whole-source checks pass:
takeover_client_pose_windows2 (six TUs), takeover_pose_info_windows1 (three TUs).
First Windows run1 preserves missing prefetch declaration, corrected with the
actual Windows intrinsic header before the helper definition.

No new runtime probes, binaries or assets transferred. Prior tests retain their
original scope; neither source compilation nor these comparison checks prove
client/renderer initialization, asset loading or Com_Frame.

Continue genuine dependency closure from link64, particularly dynamic-entity/
collision/mapkit owners, real cinematic/sound, input/network and recovery.
Keep actual system-info initialization ordering when implementing Sys_Init.
