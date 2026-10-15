# Intentional depth-prepass exclusions — 2026-10-15

`R_SetPrepassMaterial` deliberately returns false for packed prepass category2 and fading surfaces. `R_TrySetMaterialWithFunc` previously treated those returns as missing techniques and called R_WarnOncePerFrame, continually refreshing the yellow respirator-glass message.

Android now returns the original failure/skip result before warning for exactly those intentional exclusions. Technique selection, draw participation, shaders and the global warning policy are unchanged. Real missing techniques and successful fallback warnings remain active. Windows is unchanged.

The production wrapper body was extracted into an ARM64 fixture with controlled material callbacks. All64 combinations of prepass flags, fading, missing technique, regular/prepass callback and fallback availability passed on HONOR. Evidence: build-android-audit/prepass-warning-stage207/warning-policy.cpp and policy-result.txt. A pre-edit working-copy backup is preserved in the same directory.

SDK640/stage207 engine and APK build/package/install passed (BuildId5cf3c4595be1511da7e2c3d4f95bc7512652266a). Actual room/campaign-menu observation lasted69.45seconds, same PID18180,176profiles spanning61.776seconds,28.328FPS, CPUrecord25.737ms/GPU33.356ms,21passes. No fatal/assert/UBSan matches; thermal status0. Screenshots at0.9s,31.4s and62.0s show no yellow warning; room lighting, glass and moving TV images remain. This confirms the spam is absent beyond repeated5-second warning intervals; no general gameplay or performance gain claim.

Evidence: build-android-audit/native_registry_closure/sdk640-prepass-warning-a.json and its screenshots/logs. Left SDK640 running. Prior Gaussian cache and pre-existing fusion-trial settings preserved.
