# Native renderer synchronization and map re-registration

Stage51, 2026-10-15. Native gameplay/mobile performance goal incomplete.

R_SyncRenderThread checks the actual Vulkan device/physical device and attached window when the native backend is selected. The canonical main/render-thread checks, remote-update nesting assertion, FrontEndSleep and WaitRenderer remain unchanged. This is readiness admission, not a synthetic GPU completion status.

SDK566/stage135 engine01e91e619f577ed9fe5641d552f47214a130dd9c passes sync, actual Com_Init returns, map frontend starts. Map shutdown keeps the device/window as expected, but R_InitGraphicsApi relied on dx.device and attempted a second creation; native precreate correctly rejected the already-live renderer. Actual PID17181 terminated; follow confirms absence.

R_InitGraphicsApi now detects the existing native renderer, validates real device/window and NULL COM interfaces, and executes canonical R_InitSystems rather than creating another device. Initial creation still uses the original actual native startup path. Outer registration/global/water/calibration work remains. R_ShutdownStreams drains actual native frame resources before world teardown and clears native vertex bindings; no per-frame idle added.

SDK567/stage136 engine6c92e5dae29f0160cda9f3d3eb0349572eb02d53 PID18010: full native Com_Init return and actual presentation; frontend map starts; real device/window reuse and second225-sample sun calibration pass. Texture policy uses actual changing OS availability603MiB, local capacity3966MiB. Next loading-screen RB_ClearPixelShader calls old D3D setter; legacy r_setstate_d3d.h662 device assertion identifies R_HW_SetPixelShader in rb_shade.cpp, not scissor. Actual process terminated. No frontend activation/scene appearance/FPS proof.

Two ARM64 builds/packages pass; legacy r_rendercmds12758 and r_init15223tokens match checkpoint after native guards and appended owner helpers excluded. Source encodings retained. Assets reused. Evidence native-sync-stage51-build1/2.log, native-sync-stage51-launch1/2.log, native-sync-stage51-follow1/2.log, native-sync-stage51-legacy-proof.json; native_registry_closure/sdk566-sync-final-entry.log and sdk567-reuse-final-entry.log. Next actual native selected shader/declaration clear preserving immutable draw borrowers, then full frontend rendering and remaining lifecycle/performance work.
