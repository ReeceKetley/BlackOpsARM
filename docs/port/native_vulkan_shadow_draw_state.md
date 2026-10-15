# Native Vulkan shadow draw state

2026-10-15 stage59. Native backend goal remains incomplete.

SDK578 passed frontend float-depth readiness and native model-lighting patches/uploads, then a canonical shadow callback requested clear mask3 (colour+depth). The native shadow target is genuinely depth-only: the D3D NULL colour attachment has been omitted. Clearing an actual depth attachment must still happen.

The native pass adapter now removes only the dummy colour-clear bit when both colour and depth are requested on the canonical sun/spot shadow target with no colour and a real depth attachment. It still performs the depth clear. All other absent attachment aspects fail explicitly; this is not a general missing-resource bypass.

The following shadow callback calls R_HW_EnableScissor/DisableScissor. Their native branches now update actual VkCommandBuffer scissor state through the active pass recording scope. Desired driver scissor state persists across command-state and target changes, is re-applied before draws, and is clipped to actual target bounds. Signed coordinate limits reject invalid inputs; zero-area scissors are valid. Disable restores full-target coverage (the viewport still bounds rasterization).

R_ChangeDepthRange retains the canonical near/far values and state-transition assertion, then applies native viewport state rather than dereferencing a NULL D3D device. Existing Vulkan negative-height viewport conversion and depth-range validation remain active.

Engine build1 passes. Checkpoint Windows body comparison passes rb_draw3d11384 and r_state16748 tokens, CP1252/CRLF. Evidence native-shadow-state-stage59-legacy-proof.json. No original assets/providers/Vulkan version/default backend changed; no COM objects or successful frames are fabricated.

SDK579/stage148 engine959d3342f29058a5cfd1255bb341383bf8cd8862 PID15723 reaches CA_ACTIVE166043ms and the actual sun-shadow surface draw routine. The previous missing-attachment clear no longer occurs, and callback execution passes the native scissor enable/setup. Actual R_DrawSurfs then terminates at its legacy assertion dx.d3d9 && dx.device (rb_backend1637). Android exit-info matches EXIT_SELF/status2. Disable-scissor and conditional depth-range execution are not independently claimed; correct shadow pixels and a full room frame remain unproved.

Stage60 replaces that one native readiness assertion with actual device/physical-device, FrameRecording, dx.inScene, active pass scope and registered target/primitive-owner checks. Unexpected COM resources still reject the native path. The original D3D assertion remains in its branch. Canonical surface-list production/draw traversal is unchanged. SDK580/stage149 enginef08deea5afabee66fb6696293acbba8054dba7d2 PID16564 completed Com_Init and reached CA_ACTIVE167420ms. It passes the replaced R_DrawSurfs assertion and reaches actual material-state setup, then terminates explicitly: Native polygon bias needs canonical bias/scale provider. Android exit-info confirms EXIT_SELF/status2. This proves admission and traversal, not a completed submitted shadow/room frame. Checkpoint legacy rb_backend41540 tokens and CRLF proof retained in native-surface-draw-stage60-legacy-proof.json.

Correct shadow/clip/depth pixels, visible room rendering, gameplay/lifecycle and performance remain unproved; see current STATE for latest runtime evidence.

Next actual blocker: material_state.cpp rejects word1 bits0x30. Canonical R_ForceSetPolygonOffset (r_state.cpp) selects sm_polygonOffsetBias/Scale for0x30, otherwise offset>>4 times r_polygonOffsetBias/Scale; its constant bias is multiplied by0.000015258789. Implement actual Vulkan depth-bias state with the correct depth-format units, capture/cache identity and real GPU coverage. Do not simply remove the rejection or set a capability flag. Next fresh SDK581/stage150 follows a concrete changed build.
