# Native Vulkan shadow targets, stage26

Canonical R_InitShadowmapRenderTarget now attaches a real secondary Vulkan image; private owners are removed when no target references them, while borrowed GPU records retain lifetime. Native metrics select the existing R32F shader shadow technique. D24X8 probes support and falls back to D24S8, with stencil unused by the original source.

SDK518/stage87 crashed during primary allocation. SDK519/stage88 added explicit image errors and identified R32F missing linear filtering: required53377, available52615. Allocation now records actual format filtering capability; texture bindings reject unsupported linear filtering explicitly. Point-sampled R32F remains valid.

SDK520/stage89 BuildId7bb437c008b7f331b9ab6849ad3ace76a84ca215 on Adreno610 creates the canonical32x64 R32F/depth shadow target and actual VkFramebuffer; physical depth format125 (D24S8). Existing captured material and GPU state/depth probes pass. PID10562 ends1414 diagnostic submissions via BACK, exit0/resources_released0. Com_Init_called=0: shadow draw/sampling, full native startup/gameplay and performance remain unproved.

Evidence: build-android-audit/native-shadow-stage26-launch1.log, launch2.log, launch3.log, native-shadow-stage26-crash1.log, native-shadow-stage26-filter-failure.log; native_registry_closure/sdk-native-vulkan520-presentation.log and sdk-native-vulkan520-exit.log. No assets reimported, no Vulkan version change, Windows/DXVK references untouched.
