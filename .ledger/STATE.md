# Current State

Updated 2026-10-18. Zombies runs on the tablet (HONOR Pad X8a, Adreno 610) through the native Vulkan backend at 540p + SGSR, about 30 FPS in the heaviest Kino view (GPU bound). The user accepted that as the device limit and paused optimisation. Two commits exist (802782ef, f9b8e33e); f9b8e33e is pushed to remote `arm` branch android-arm64 (main there is the older mirror). Later work is uncommitted. Commit or push only when the user asks.

Older detail through 2026-10-17 is in .ledger/archive/STATE-through-2026-10-17.md. Resolved bring-up failures (FAIL-0001..0241 except 0239) are archived in the ledger (hidden, not declared false); a full ledger backup was taken first (.ledger/archive/backup-20261018*).

## Working now

- Rendering and performance: parallel scene recording (2 workers, pinned to fast cores), sharded binding registries, static-model cache NOOVERWRITE patch, program cache 1024, pipeline prewarm. Diagnostics kept (log slow frames only): native_frame_spike, native_main_spike, native_handoff_wait, native_backend_spike, native_parallel_record.
- All 10 zombie maps start from the launcher; map sweep tool: tools/port/run_map_sweep.py (evidence under build-android-audit/map-sweep-*).
- In-process launcher (committed f9b8e33e): the WebView menu (assets/launcher/index.html) is an overlay inside EngineActivity (:engine process). LauncherActivity is only a router. Native launcher_session publishes state (Booting/Menu/Loading/Match) and takes map requests; the WebView is destroyed at Match and recreated at #maps when the engine returns to the frontend. New JNI symbols must be listed in src/platform/android/native_engine_exports.map. Quit from the pause menu returns to the launcher map select. A map started after a match boots a fresh engine behind the page loading screen (Host.needsFreshEngine) because a second map load in one process fails with "Native vertex buffer load: owner already loaded" (buffer_assets owners are not released at map unload; OPEN).
- Real load progress (uncommitted): db_file_load counts compressed 256 KB reads of the chosen map's zones (Android_NoteLoadChunk), Java divides by those zones' file sizes (EngineActivity.prepareProgress).

## Controller and input (uncommitted, this session)

- Controller detection works (SDL game controller; "Generic X-Box pad" 24c6:581a). Raw trace logging CONTROLLER ... kept in controller_service.cpp (first 600 changes per run).
- gpad_enabled now follows the attached pad (win_gamepad.cpp GPad_Check, Android only) and the pad's button and stick config files are exec'd on attach. Without this nothing but the dev-menu shortcut reacted.
- Both-stick-click dev menu shortcut disabled on Android (cl_gamepad.cpp); the `devgui` console command still works.
- Fixed a 32-bit leftover that crashed the UI when a slider was dragged: Scroll_Slider_ThumbFunc read p[6] instead of scrollInfo_s::item (ui_shared.cpp/.h).
- Fixed an assertion crash when pushing the right stick diagonally (aim_assist absYawAxis <= 1.0): CL_GamepadAxisValue now clamps to +-1 (Android) and the launcher look speed scales the resulting turn in CL_GamepadMove instead of the stick axes. NOT yet confirmed on the tablet (SDK765/stage331 was deploying).
- Launcher input settings wired: button layout (buttons_default/experimental/lefty/nomad/charlie), stick layout (thumbstick_default/legacy), horizontal and vertical pad sensitivity (dvars bo1_pad_yaw_scale / bo1_pad_pitch_scale, default vertical 0.8 because it felt faster), invert look (input_invertPitch), touch look speed (Java, EngineActivity.touchLook). Still marked "soon": vibration (the engine never drives pad motors; only SDL rumble service exists) and gyro aim.
- Temporary diagnostics to remove once input is verified: Java Log.i INPUT/LIFECYCLE lines (EngineActivity), "ENGINE quit requested" print in Sys_SetQuitRequested (quit_services.cpp), CONTROLLER raw trace.
- Wireless debugging works (adb pair/connect). tools/port/run_native_vulkan_engine.py honours BO1_ADB_SERIAL (default is the USB serial). tools/port/native_console_fifo.py still defaults to the USB serial; pass --serial or write to files/user/native-console.fifo with run-as (commands need the slash prefix).

## Open issues and watch items

- FAIL-0243 / FACT-0568: intermittent heap corruption class. One SIGABRT in free() under vkDestroyDescriptorSetLayout (ProgramCache::Clear via EngineFrames::Drain) after pause-menu Quit; not seen again after the R_UnlinkDynEnt null-world guard. FAIL-0242: SDK680 post-publication pixel shader corruption (intermittent).
- FAIL-0239: canonical Com_Quit reaches an unimplemented Android splash owner (pause-menu Quit to the menu works).
- Open: buffer owner release at map unload (see above); Moon sidequest script error (not precached p_zom_moon_cassimir_plate); Shi No Numa/Verruckt missing hud_zombie_cod5_* assets (believed optional, unverified); zombie path-freeze (FACT-0561/0563); strict WMA host test (native_wma_decoder_tests.cpp) not re-run.
- Quit and menu return crash fixes are in; one engine exit with status 2 and no message was seen during controller testing before the axis-assert fix (the same repro now explained by the assert).

## Tooling notes

- Deploy: PowerShell, Set-Location repo root, Remove-Item Env:NoDefaultCurrentDirectoryInExePath, then python -I tools/port/run_native_vulkan_engine.py --sdk N --stage M --build build-android-audit/native_engine_perf1 --height 540 --seconds 5 (fresh SDK and stage numbers each time; next unused are 766/332). Header changes in ui_shared.h or similar rebuild for several minutes; run in the background.
- Heredocs through the Bash tool collapse backslash escapes (\n inside C++/Python string literals becomes a real newline); use the Edit tool or a Write-d script for such edits. Some source files are cp1252 (ui_shared.cpp, sv_msg_write.cpp): edit as bytes.
- Windows reference checkout ../BlackOps64 is read-only; keep ../DXVK-Native and retail assets untouched.
