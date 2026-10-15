# Native backend scene transactions

Stage48, 2026-10-15. Goal remains incomplete.

Canonical RB_BeginFrame now obtains the actual device-owned EngineFrames and begins an offscreen recording transaction. The in-scene flag publishes only after actual recording succeeds. Existing cinematic/cache/model updates and command setup remain. Canonical command completion has a native branch to finish the pass, submit real Vulkan commands and wait for the actual GPU fence before clearing in-scene. This initial synchronous policy is for integration, not a mobile performance optimization. Queue ownership/order failures remain explicit. The end branch compiled but full-engine execution has NOT reached it yet.

Native R_InitCmdBufSamplerState resets decoded filter/address settings, recreates Vulkan samplers for already-bound images, retains image/mip-bias snapshots and updates canonical reference-state arrays. Unbound slots retain metadata until a real image binds. No fake texture/interface or synthetic D3D status. This reset preserves already-captured immutable draw packets. Bound-image reset GPU execution is not separately proved.

SDK561/stage130 engine9485e0537f5b0713c06473139d9414096c7b1ba2 passed BeginFrame device assertion and reached sampler reset nullD3D SetSamplerState. SDK562/stage131 enginea04b1e31546008fd23869c8b394fc634f6810fed passed native begin and sampler reset, then failed R_ChangeStreamSource legacy r_state.cpp1561 device assertion during command setup. Actual PID13935 terminated; follow observer confirms no process. Original assets reused. Com_Init return, engine scene submission, native window output, frontend/gameplay/FPS remain unproved. Window presentation still needs replacement; no silent offscreen-success claim.

Both ARM64 builds and exact seven-provider package checks pass. Legacy source tokens match checkpoint after native guards removed: rb_backend41540, r_state_utils3065. Evidence: build-android-audit/native-frame-stage48-legacy-proof.json; native-frame-stage48-build1/2.log; native-frame-stage48-launch1/2.log; native-frame-stage48-follow1.log; native_registry_closure/sdk561-frame-final-entry.log and sdk562-frame-final-entry.log.

Next: trace the actual stream-clear caller with matching ELF/maps, extend owner-aware native stream clearing/binding as required, then real draw/end submission/window presentation. Avoid treating COM NULL as a resource owner. Packed GPU decode/model-atlas updates/intbool banks/lifecycle/performance are still pending.
