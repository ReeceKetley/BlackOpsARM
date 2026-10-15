# Native Vulkan polygon depth bias

2026-10-15 stage61. Full backend/mobile performance goal remains incomplete.

SDK580 reached real surface traversal, then material_state rejected canonical word1 polygon offset bits0x30. The implementation now decodes those bits without pretending a GPU pipeline is resolved. Such a description remains explicitly unresolved until draw capture supplies actual engine settings and the selected pass supplies its real depth format. GraphicsProgram::Create and PipelineKey::Create both reject unresolved or non-finite bias.

Draw capture copies the canonical selected dvars into immutable normalized bias/slope values: mode3 selects sm_polygonOffsetBias/Scale once, modes1/2 multiply r_polygonOffsetBias/Scale by the mode. The constant multiplier0.000015258789 matches R_ForceSetPolygonOffset. Later dvar changes do not modify retained draw packets.

ResolveDepthBias converts normalized constant bias to fixed depth units: D16 times65536, D24 times16777216. This matches the validated DXVK-Native GetDepthBufferRValue/BindDepthBias convention, inspected read-only. The actual pass format travels through material_dispatch::Pass. Both resolved factors and enable state participate explicitly in cache hashing/equality. The actual Vulkan rasterization pipeline sets depthBiasEnable, depthBiasConstantFactor and depthBiasSlopeFactor; clamp remains0, requiring no optional clamp feature or newer Vulkan.

Only the currently used fixed-depth families are admitted for biased draws. Floating-point constant bias has depth-exponent-dependent semantics and remains an explicit unsupported policy; no guessed format or shader skip is used. Bias-disabled draws require no depth format.

Vulkan specifies implementation-dependent fixed-depth resolution within its limits, so exact output across GPUs is not promised. Sources: [depth bias computation](https://docs.vulkan.org/spec/latest/chapters/primsrast.html), [depth-bias-control motivation](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_depth_bias_control.html). The test permits the documented resolution range and records actual pixels rather than asserting pixel identity across hardware.

## Tablet evidence

native-bias-stage61-tablet1.log and native_vulkan_shader3/manifest-bias61-1.json: ten actual Adreno610 draw/fence/readback cases across D16 and D24. Each format tests disabled, positive and negative normalized constant1/1024, positive and negative slope1 on a known sloped triangle. Real sampled depth feeds a float RGBA target and host readback after the actual fence. All pixels/channels are checked. Reused shader bodies are not a new compiler feature.

D24 first pixel: disabled .50000006, positive .500976622, negative .499023467. Slope+ first/last .34375003/.53125006; slope- .218750015/.40625003. D16 differs only by its expected quantization tolerance. Pipeline-key checks distinguish changed constant and slope; unresolved cache insertion and absent/floating formats reject.

The focused --bias-only path runs this family only, avoiding historical24 shader/math and6 depth-comparison repeats. It transfers only changed harness/compiler binaries, no assets, and does not restart the game.

Engine build1 passes. SDK581/stage150 engine9a9a4c976422a752f89365e7e7cb69c08e7c3667 PID17644 passes Com_Init and CA_ACTIVE, admits the canonical polygon state and reaches BSP world-geometry production. It then fails the old R_ChangeStreamSource D3D device assertion (canonical line1561). The matching staged ELF/maps symbolize R_SetStreamSource -> R_SetStreamsForBspSurface -> R_DrawPreTessTris -> R_DrawBspDrawSurfsPreTess -> R_TessTrianglesPreTessList -> R_RenderDrawSurfListMaterial. Actual full-game biased draw submission was not reached by this run; the ten focused GPU cases are the depth-bias execution proof. Stage62 adapts that world producer to native buffer owners.

Correct full shadow/room pixels, gameplay, lifecycle and performance remain unproved. Canonical Windows source bodies and original retail assets were not edited.
