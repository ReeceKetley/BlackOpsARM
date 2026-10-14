# Native tablet touch controls

Implemented genuine tablet controls for the existing rendered engine input path. Production is frozen for root packaging; this agent performed no device, APK, full engine build, asset, provider, config, Ledger, STATE, or commit actions. Actual title/menu/Zombies input and visual control layout on HONOR remain runtime validation for the root owner.

## Behavior

Touch controls default to visible and persist their TOUCH ON/OFF preference. A transparent Android View owns its multitouch streams, preventing SDL's primary-finger mouse1 route from also firing. Hiding controls restores the existing SDL primary-touch path. Physical mouse events pass through the View; keyboard/controller routes continue unchanged. Metrics are raised above the control layer and Volume Up retains its toggle.

In gameplay the left pad retains/releases W/S/A/D, including diagonals. Dragging open space supplies normalized relative look deltas. The right buttons use the canonical defaults from common.cpp: MOUSE1 attack, MOUSE2 toggleads_throw, SPACE gostand, R reload, C togglecrouch, F activate, SHIFT breath_sprint, 1 weapnext, V melee and G frag. MENU uses ESCAPE. Existing canonical key bindings determine engine commands, including any user remapping; this implementation does not execute an independent command or gameplay engine.

When the real client is not CA_ACTIVE with no key catchers, the pad sends the canonical arrow keys, OK sends ENTER, MENU sends ESCAPE, and open space supplies cursor position/MOUSE1 clicks. The menu route also supports title/Press Any Key. Native admission is independent from the visual overlay: TOUCH WAITING labels lack of real owner admission. The UI cannot announce engine readiness.

## Ownership and bounds

JNI methods only read/write a mutex-protected bounded mailbox and copy primitive array values. They call no engine/SDL APIs and retain no Java objects. The actual installed SDL frame owner queries running/active state, mouse policy, focused/foreground/nonclosing attached window, actual positive logical/drawable geometry, and canonical client/key-catcher state before publishing an admission epoch.

The mailbox preserves at most 64 ordered transitions, below the canonical 256-event system queue capacity. Complete down/up taps between frames remain ordered. Duplicate fingers sharing a UI action use Java action refcounts; owner touch holds use the existing retain/release canonical-key counts with physical keyboard/mouse/primary-touch sources. Touch cancellation releases only its own contributions. Existing gamepad processing remains canonical and unchanged.

Adjacent menu pointer motions coalesce without coalescing across button transitions. Look deltas coalesce as finite normalized doubles, convert using actual drawable dimensions into the same signed integer pixel units as CL_MouseEvent, and retain subpixel carry within an epoch. Producer and combined canonical mouse capacity checks prevent unsafe narrowing/overflow.

Queue pressure invalidates admission, drops the pending batch, and requests owner release of touch holds; it does not overwrite individual release transitions. Focus loss, background, inactive input, shutdown, geometry/mode change and UI hide/pause/surface-loss/cancel invalidate epochs and remove queued transitions. A fresh owner frame is required to re-admit. Owner lifecycle releaseAll also clears touch ownership. Touch input never directly releases a physical hold from the UI thread.

## Verification

- The actual Android rendered_input TU compiles using its production compile_commands NDK30/API28 flags, instrumentation and UBSan (exit0). The full actual app Java sources plus matching SDL2.30.11 Java sources compile against android35.jar with Java17 (exit0). Expected pre-existing engine header/deprecated SDK warnings remain.
- A host fixture extracts the exact production mailbox/JNI/owner/key-retention functions, with explicit fixture SDL/window/event dependencies. It passes 100 checks covering pre-init rejection, between-frame tap order, overlapping physical/touch holds, queued cancellation, stale epochs, input/nonfinite bounds, drawable delta units and subpixel carry, geometry/focus/menu/shutdown invalidation, menu navigation/cursor, 64-event pressure, coalesced overflow, mouse disable and real two-thread producer/owner contention. This is synchronization/ownership evidence, not real SDL/device gameplay proof.
- Native object contains all four new JNI definitions. Export map adds exactly these four and preserves the three original activity exports. No CMake changes are required: rendered_input.cpp is already referenced by the canonical frame owner and retained in the engine link.
- Exact source body comparisons preserve all existing SDL keyboard, text, mouse, wheel and primary-touch consumption, translation/refcount helpers and lifecycle routines. FrameRenderedInput changes only by adding frameTouch; metrics/native-bootstrap behavior and relevant original Java methods are preserved.
- Production files are byte-identical to the compiled candidates. UTF-8 decode, CRLF, original no-BOM encoding, source baseline and frozen SHA256 are verified in source-manifest.json. No retail/shared Windows source changes.

## SDK466 frozen SHA256 (historical implementation)

- src/platform/android/rendered_input.cpp: `3FA0553A8E2369A729220C757AD1405369D9DC49B3846F74A6FE1ADE1419C380`
- android/app/src/main/java/com/blackopsarm/dev/EngineActivity.java: `8CAB5231AFC9ABBA4E7B26DA6A0A4138BBDBA54E8E0DDFBF7B95993099C7FF69`
- src/platform/android/native_engine_exports.map: `20B5BF95B90E6C42A5E1AA174D4222A32F760D4F9311445B5FD8AA78448FFDE9`

Evidence: build-android-audit/native_touch_controls1/{source-manifest,compile-results,mailbox-results,final-check-results}.json, native/java compile logs, exact-source fixture/run log, candidate/pre-edit copies and reproducible scripts.

## SDK467 cursor event ordering repair

The first implementation applied only the final menu cursor coordinate in FrameRenderedInput before Com_EventLoop dispatched all queued SE_KEY events. Two complete taps at A/B between frames therefore both clicked B; a tap at A followed by hover B also clicked B. The SDK466 implementation and evidence remain preserved in native_touch_controls1 and the verbatim pre-repair report in native_touch_cursor_order1/native_tablet_touch_controls.md.before.

The native provider now queues each hover/cursor-down/cursor-up as a single Android-private scalar event (types4/5/6), with drawable x/y in evValue/evValue2 and admission epoch in evPtrLength. evPtr is always null. The native-guarded Com_EventLoop hook recognizes these events before the unchanged canonical switch. The installed SDL owner validates actual admission epoch, running/input/window/focus/geometry/menu state at dispatch, then applies canonical CL_MouseEvent immediately before the corresponding canonical CL_KeyEvent edge. Hover events after a tap stay after that tap. Cursor touch key ownership begins at event consumption; canceled pending cursor events never retain a key. Applied touch cursor holds share the existing held refcounts and lifecycle release paths with physical mouse input.

Canonical mouse/UI APIs run on the real event consumer, outside the producer mailbox lock, so synchronous engine UI-to-Java calls cannot deadlock against Java cancellation. The Java UI remains only a bounded producer. No Java, JNI export-map, core enum, sysEvent_t layout, payload allocator or Windows source behavior changes are needed. Gameplay/nav input and coalesced relative look remain on their existing paths.

An exact two-line source-specific CMake property enables BO1_NATIVE_RENDERED_INPUT for common.cpp only inside the existing BO1_NATIVE_SDL_APP block. The runtime common TU otherwise lacks the database target's definition. Root must regenerate its actual master-build compile_commands and verify this flag and linked hook when packaging SDK467. No target-wide definition/provider/options changes were made.

Scalar field safety was checked against the immediate canonical contracts and all repository evPtrLength references. Sys_QueEvent merely stores the fields; actual Android Win_GetEvent copies/clears records; Sys_GetEvent is a direct record copy. Overflow and Sys_ShutdownEvents free only nonnull evPtr. LoadingKeepAlive drops/free-checks only evPtr, and DB_Sleep drops the copied event without reading payload length. This repository implements no system-event journal or generic payload-length serialization/reader. Thus the provider's private epoch field cannot cause a null payload copy or leak allocated memory in these supported paths. Existing event overflow semantics remain canonical.

Production rendered_input and common-with-hook compile with the actual NDK/API/debug/instrumentation/UBSan flags (both exit0). Macro-disabled before/after preprocessed Com_EventLoop bodies are identical. The focused host fixture extracts the exact production native functions and actual Sys_QueEvent, Win_GetEvent, Sys_GetEvent, Sys_ShutdownEvents and Com_EventLoop bodies, with explicit fixture dependencies. It passes34checks: A/B taps, tap-then-hover order, ownership deferred until consumer, enqueue-then-cancel, actual mode/focus/geometry/retirement rejection, overlapping physical mouse holds, applied-hold cancellation, actual 256-record overflow and retirement without allocation/free. This fixture is not tablet gameplay proof. Java is unchanged, so its prior compile remains applicable; the prior100ownership checks retain their historical scope.

The source byte inverse confirms common.cpp differs only by the guarded include/hook, and the recipe differs only by the authorized property. Original native cpp CRLF, native header LF, common.cpp mixed endings/CP1252 projection, CMake LF and BOM status are preserved. This amended report and JSON evidence are written/decoded as UTF-8 and verified before replacing prose.

Related limitation retained for root coordination: a later owner frame's stationary SDL mouse fallback can restore the physical cursor position when no new touch cursor event arrives. Ordered clicks are fixed, but persistent hover continuity between touch events requires suppressing that fallback until real physical mouse motion or epoch/lifecycle reset. No post-freeze production change was made for this observation.

### SDK467 frozen sources

- src/platform/android/rendered_input.cpp: `991A431B87FB5304AC58090E3F514C7E208B45CF32962980AD1F9D6ACCB3E337`
- src/platform/android/rendered_input.h: `04F30C4EDA73CAA55EF5633BB6EBE1B2FBE9BCC96725ADCF48242FAE332131C3`
- src/qcommon/common.cpp: `EA90D9D0C1BC8810D6F1581D10B47F79F1E767F1FDC2338D39B0FA638DD93620`
- android/cmake/native-engine-entry/CMakeLists.txt: `7658D07B6F741DB456B2B03A0C1C85D26F75CEA76FF6E4FBACCD87FAA7B612FC`

Evidence: build-android-audit/native_touch_cursor_order1/{source-manifest,compile-results,ordered-results,final-check-results}.json and exact-source ordered-fixture/run/compile logs, baseline copies, scripts and amended-report prior version.

## SDK468 actual frame closure, recipe scope and cursor continuity

SDK467 master regeneration disproved the original CMake target assumption: its only actual common.cpp compile entry is startup-services/CMakeFiles/bo1_native_startup_services.dir/.../common.cpp.o, and BO1_NATIVE_RENDERED_INPUT was absent. The initial parent-directory source property did not affect the target created by native-startup-services add_subdirectory. The earlier explicit -D candidate compile remained valid as a candidate syntax proof, but the packaged SDK467 dispatch hook was compiled out; no successful custom pointer dispatch runtime claim is made. Root avoided these private pointer events until the corrected APK. The wrong recipe version and previous report are preserved in native_touch_cursor_order1 and native_touch_cursor_continuity1/CMakeLists.txt.before.

The corrected source property adds TARGET_DIRECTORY bo1_native_startup_services, leaving all other recipe bytes unchanged. The actual SDK CMake3.22.1 set_property help confirms this option sets source-file properties in the specified existing target's creation-directory scope. Root must verify the regenerated common.cpp command contains BO1_NATIVE_RENDERED_INPUT=1 and the linked hook in the SDK468 master build. This source-specific correction does not broaden target compile definitions.

Separately, the actual title screenshot showed TOUCH WAITING while physical Enter reached the canonical event loop. Source callgraph inspection explains this: RunNativeEngine's post-Com_Init loop called only Platform_InputFrame. On Android that global function is defined in console_input.cpp and pumps SDL/console/renderer-owner requests; it does not dispatch the IInputBackend Frame method. NativeInput::Frame could call IN_Frame_Native, but no Android global backend dispatch reaches it. Thus no actual FrameRenderedInput/frameTouch invocation consumed per-frame mouse/controller state or published touch admission, despite the real IN_Init owner being installed.

The rendered full-database outer loop now calls the existing canonical IN_Frame_Native, which requires its initialized owner, pumps Platform_InputFrame once, and calls FrameRenderedInput once for nonheadless input. The full-database guard includes system_initialization.h for the canonical declaration. Headless entries keep their original Platform_InputFrame branch. The exact source-byte inverse removes only this header and rendered branch; the root MSAA-off commandline, trace/diagnostics/policies, all unrelated flags and original mixed LF/CRLF encoding remain unchanged. Readiness still requires the real owner/window/geometry/Java gate; no focus or readiness value is forced.

The source-extracted host closure fixture includes the actual rendered outer-loop body, IN_Frame_Native, console Platform_InputFrame, FrameRenderedInput and frameTouch. Six checks prove initialized-owner admission precedes pumping, a rendered iteration performs one SDL pump and one rendered input frame without recursion, frameTouch admission occurs before Com_Frame, headless performs only its original console pump, and invalid owner admission is rejected before SDL input. The actual runtime TU compiles using its production command/NDK/debug/UBSan flags (exit0). Fixture dependencies are explicit; genuine tablet readiness remains to be verified in SDK468.

Cursor continuity is now integrated: an owner-only flag records an accepted menu touch cursor position and suppresses stationary SDL_GetMouseState fallback until real physical mouse motion or an epoch/lifecycle release clears it. This prevents the subsequent stationary frame from jumping a valid touch hover back to the previous physical cursor. The real physical mouse-motion branch clears the flag, retaining its original scaling/delta/dispatch behavior. The nine-check exact-source candidate fixture covers next-frame hover persistence, physical motion taking cursor control back, touch reclaiming it, cancellation restoring fallback, and menu-to-gameplay reset. Candidate native compilation passes; production is byte-identical to that compiled candidate. No Java, export-map, common or header change was added for continuity. The previous stationary-hover limitation is resolved at source/fixture scope, while live APK verification remains pending.

All SDK468 sources below are frozen for root packaging. No additional device/master build/asset/provider actions were performed by this agent. Root owns actual generated-flag/link checks and tablet title/menu/Zombies/touch/focus/pause/surface/controller validation; source and host fixtures do not establish playable runtime success.

### SDK468 frozen SHA256

- src/platform/android/rendered_input.cpp: `40B5FC5F4E38B781C5900580C95FF08A02B354E69ED57C0E460100045414E8D1`
- src/platform/android/rendered_input.h: `04F30C4EDA73CAA55EF5633BB6EBE1B2FBE9BCC96725ADCF48242FAE332131C3`
- src/qcommon/common.cpp: `EA90D9D0C1BC8810D6F1581D10B47F79F1E767F1FDC2338D39B0FA638DD93620`
- src/platform/android/native_engine_runtime.cpp: `7F90B1E340E8BB0F5357E3E8CFB416FE34AB87C7D21893C9A8ED51B616ADC353`
- android/cmake/native-engine-entry/CMakeLists.txt: `E9B0D1FC83956D74C37DF2F7AA298DCEFF95A791E27DFAFA8EBA7E71BD5AB1D6`
- android/app/src/main/java/com/blackopsarm/dev/EngineActivity.java: `8CAB5231AFC9ABBA4E7B26DA6A0A4138BBDBA54E8E0DDFBF7B95993099C7FF69`
- src/platform/android/native_engine_exports.map: `20B5BF95B90E6C42A5E1AA174D4222A32F760D4F9311445B5FD8AA78448FFDE9`

Evidence: native_touch_cursor_continuity1/{recipe-manifest,source-manifest,candidate-results}.json and continuity-fixture logs; native_touch_frame_closure1/{source-manifest,final-source-manifest,check-results}.json, runtime-native and frame-closure-fixture logs, full prior report and source snapshots.

## SDK469 ordinary menu IME correction

Root verified the corrected common.cpp BO1_NATIVE_RENDERED_INPUT definition in SDK468's actual startup-services compile command. The same genuine engine process now reaches visible 3D interrogation-room/menu rendering with live metrics after physical Enter. Root also observed that actual FrameRenderedInput was now opening Android's IME on ordinary UI menus, obstructing the lower touch controls. Source/input fixes remain distinct from playable Zombies or performance-target completion; no additional device test was performed by this source agent.

The prior text-input predicate treated every UI key catcher0x10 as editing. The native owner now includes the canonical ui/ui_shared.h declaration and requests text input only for console catcher1, chat catcher0x20, or an unblocked UI catcher0x10 with canonical g_editingField active. UI catcher0x100 blocks the UI character path exactly as in CL_CharEvent. Item_TextField_BeginEdit sets g_editingField; the menu text-field handler clears it when edit handling ends or a pointer transition exits editing. Ordinary menu focus alone therefore stops or leaves SDL text input inactive. Actual text fields, console and chat retain text input, including when touch controls/mouse admission are unavailable. Focus/foreground/closing-window checks and SDL start/stop transition idempotency are unchanged.

Only rendered_input.cpp changes: one canonical include and the text-input predicate. Native compile with the actual NDK/API/debug/instrumentation/UBSan command passes. A focused fixture extracts the exact production FrameRenderedInput body and passes17transition checks covering ordinary menu no-start/old-IME stop, UI edit begin/end, repeated-frame idempotency, console/chat, bind capture, stale/non-character catchers, focus/background/closing rejection, and real UI editing with mouse admission disabled. Separate exact source proofs establish the canonical global's begin/end state and CL_CharEvent routing. The original full-fixture suites and unchanged Java/export/runtime recipe are not rerun. The byte inverse verifies no other native input behavior changed; UTF-8, CRLF and BOM status are preserved.

SDK469 frozen rendered_input.cpp SHA256: `F49B416BCF20FEE9DD18F4C616980E46D3ECA527E0B535AD7BE78DC124D8BAE6`. Before: `40B5FC5F4E38B781C5900580C95FF08A02B354E69ED57C0E460100045414E8D1` (SDK468).

Evidence: build-android-audit/native_touch_ime_predicate1/{source-manifest,check-results}.json, native/ime-fixture logs, baseline source and verbatim prior report. This report amendment is written/read as UTF-8 and verified before replacing prose. Actual SDK469 IME, touch/menu/gameplay and lifecycle validation remain root-owned; the running SDK468 process is unchanged by these source edits.


Root actual SDK469/470 validation: tapping the observed on-screen OK button
passes the genuine title into visible 3D room rendering. Ordinary menus leave
Android's keyboard closed, while opening the real console requests text input
as intended. SDK470 room-clean1 captures the same process with console/keyboard
closed and fresh FPS/CPU/GPU/RSS. These are actual touch-title and IME gates,
not gameplay movement/look/actions or full lifecycle verification.
