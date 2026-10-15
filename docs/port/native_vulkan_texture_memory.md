# Native texture policy and startup registration

Stage47, 2026-10-15. Full native replacement remains incomplete.

The canonical automatic picmip policy now obtains a measured Vulkan/Android texture allowance. The native service queries actual device-local heap capacity, advertised VK_EXT_memory_budget when present, and /proc/meminfo MemAvailable. UMA is capped by known OS availability. Unknown capacity/availability fails explicitly. The allowance is a policy estimate, not an allocation guarantee or exact free GPU memory. This tablet lacks the budget extension, so the observed source is heap capacity bounded by OS availability (local heap3966MiB; allowance approximately1057–1095MiB, varying with RAM).

R_SetPicmip validates the actual native device instead of a D3D interface; existing automatic/manual quality policy remains. R_DrawTessTechnique requires live recording/pass ownership. Renderer registration validates the actual device and attached window. The sun calibration executes canonical geometry, submits commands, waits for the actual GPU fence, and reads225 samples on SDK559. No samples are fabricated. R_Init and renderer registration advanced; actual Com_Init returned is NOT proved.

SDK557/stage126 reached the tess technique D3D check. SDK558/stage127 reached the registration D3D check after R_Init. SDK559/stage128 executed sun calibration then hit RB_UpdateBackEndDvarOptions TestCooperativeLevel on null D3D. SDK560/stage129 adapts that CPU-side option update to actual native device/window readiness and reaches RB_BeginFrame's legacy device assertion. PID12616 subsequently died with a fatal assertion; timeout alone was not treated as death. ARM64 build/package provider verification passed. Engine SDK560 BuildId16350fb089618c587c59e3d0e98d4d5fc6ac2582.

All six affected legacy source bodies match checkpoint tokens with native guards removed: texturemem1225, image8358, shade2674, sky5663, init15223 (appended native helpers excluded), backend41540. Source encodings/CRLF retained. No asset transfers or original shader/asset edits.

Evidence: build-android-audit/native-memory-stage47-legacy-proof.json; native-memory-stage47-launch1..4.log; native-memory-stage47-follow1.log; native_registry_closure/sdk-native-entry560-follow1-only.log. Existing SDK557/558 final-entry logs retain failures. Next real native begin/end command transactions and window presentation, followed by frontend/gameplay/lifecycle/performance. Model-light patches, packed GPU vertex decode and int/bool engine banks remain pending.
