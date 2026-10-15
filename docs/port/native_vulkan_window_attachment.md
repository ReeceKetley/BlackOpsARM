# Canonical native window attachment, stage39

Native device service records its actual SDL window/id at Open. AttachWindow validates creator SDL thread, same live window identity/Vulkan flag, real populated swapchain/renderpass/framebuffers and exact positive drawable dimensions. WindowAttached is association ownership, not perpetual surface readiness; future resize/surface lifecycle still needs engine integration.

R_FinishAttachingToWindow native branch validates an empty engine window slot and real attachment, then publishes HWND-as-actual-SDL-window/width/height/windowCount1/targetIndex0. COM swapchain pointer stays null; no fake interface or assertion bypass in legacy path. Device Close unpublishes its matching engine slot/count through R_DetachNativeWindow before presenter/device/surface retirement and caller SDL destruction. Conflicting engine window ownership fails explicitly.

SDK537/stage106 BuildId62278c455604d533f1497d16e114823c76ec8eda PID22641 actual canonical attach logs1920x1200/swapchainreal/COMfalse. Existing initial state/query families/precise query/event/generated targets/material dispatcher GPU checks pass. BACK1221 submissions exit0; canonical detach logs engine_window_count0, resources_released0/USB0. ARM64 build/legacy existing r_init tokens pass.

This is window association used by diagnostic presentation, not full native engine hardware/frame startup. Com_Init_called=0. Full R_InitHardware/Com_Init/engine frame transactions/partial submit/startup calibration/uploads/packed vertices/timestamps/native teardown/reset/gameplay remain pending. Invalid/double attachment, failure recovery, resize and reattach runtime coverage not claimed. D3D9 remains full rendered default; assets and Windows/DXVK references unchanged.

Evidence: build-android-audit/native-attach-stage39-launch1.log, native-attach-stage39-legacy-proof.json; native_registry_closure/sdk-native-vulkan537-presentation.log and sdk-native-vulkan537-exit.log.
