# Canonical engine Android APK integration

The development APK now packages the actual native engine. The CLI and exported
SDL_main call one RunNativeEngine body and link the same PIC production archives.
It does not contain an independent engine or report unavailable providers ready.

The source build enters src/platform/android/CMakeLists.txt, retains the genuine
environment probe and adds the established native-engine-entry recipe with FULL
database and SDL app enabled. BO1_NATIVE_SDL_RENDERED_ENTRY applies to its actual
win_main policy owner. SDK admission requires the real Activity adoption release
publication; the console retains dedicated behavior. Original assets, fixed32
disk/VM layouts, Windows reference and pinned DXVK/SDL providers remain intact.

The shared library exports SDL_main and the actual EngineActivity adoption JNI
entry. Other engine symbols are internal. NativeUi callbacks use the existing
real RegisterNatives function pointers. --no-undefined remains enforced. Initial
unrestricted exports retained three otherwise unused internal bodies with missing
references; restricting the external ABI removed those dead roots without adding
fake implementations or removing reachable engine behavior.

## Packaging and verification

The seven ARM64 libraries are engine, environment probe, SDL2, DXVK D3D9, Steam
API, native shader compiler and NDK UBSan. stage_native_engine_apk.py checks actual
ELF architecture, SDL/JNI exports and every recursive non-system dependency, then
copies exact bytes and records SHA256/BuildId. Source CMake explicitly imports
the NDK sanitizer provider for packaging. Static C++ is retained.

Normal source build:

```powershell
./android/gradlew.bat -p android :app:assembleDebug
```

To reuse the actual production build already compiled for tablet integration:

```powershell
python tools/port/stage_native_engine_apk.py --build build-android-audit/native_engine_entry1 --output build-android-audit/apk-native-stage2
./android/gradlew.bat -p android :app:assembleDebug '-Pbo1NativePrebuiltDir=D:/Desktop/Black Ops 2 Android/BlackOpsARM/build-android-audit/apk-native-stage2'
```

The explicit staging option avoids recompiling 714 database TUs in a second
Gradle build tree. APK1 assembled in22s, APK2 in7s; both install successfully on
the HONOR tablet. aapt reports arm64-v8a and AssetImportActivity as launcher.
Actual UI shows import prerequisites and disables game launch without a committed
receipt. EngineActivity runs in its own :engine process because native global
owners initialize once and canonical quit is terminal. Activity recreation,
rendered native boot, audio/input/GPU and gameplay remain unverified.

## Real storage integration

User explicitly requested existing ADB assets be moved beside Download, to
/sdcard/boARM. The local move checks literal source/target, actual shared-root
resolution, target absence and no live engine, then moves the existing tree.
It completes exit0; /data/local/tmp/blackopsarm-game-data no longer exists and
/storage/emulated/0/boARM is present. No original game files were sent from the PC.
Future native development uses the new shared source root.

The real Android folder picker selects boARM and grants persisted tree access.
The first import attempt caught a genuine Java path bug before copying: normal
Context /data/user/0 ancestor aliases were rejected. The correction canonicalizes
the trusted Context base before deriving guarded children; child links/traversal
remain rejected. SDK compile and production path checks pass. APK2 contains that
correction; the second real import is now running and writing its incomplete
receipt. Completion, complete receipt and actual SDK engine execution still
require observation. The source boARM remains user accessible; the managed APK
copy belongs to the application, preserving originals and real import semantics.

Retained evidence: native_registry_closure/sdk-engine-shared-build1/2/3.log,
full-engine-link430.log, sdk-apk-build1/2.log and sdk-importer-ui1..9.xml;
apk-native-stage1/2/native-manifest.json; asset_import_java/source-root-fix-manifest.json.
User-root and asset-import implementation details are in native_android_asset_import.md.

## Actual completed import and first native SDK boot

Second real SAF import completes all266files withcommittedprivate receipt;
actualUI shows266copied/0skipped, sourcepreserved andLaunchenabled. APK2/native
stage2 sevenSO bytes match exactly. FurtherAPKupgrades preserve the committed
assets; no reimport/hash sweep occurred. ActualSDK431 launches SDL_main, validates
realSDKActivity androots, captures privateuserlog andcallsactualCom_Init. DXVK
detectsAdreno610/Vulkan1.1.128 andshader3.0, thenrendererthread abortsdisplay-mode
enumeration. This provesapp/native/asset/providerintegration, notrenderedframes.
MatchingSO BuildId586258f5196c0d0007a9a05a02554a8e4ef94128; logs retained.

User changedactiveobjective to rendering/gameplay, thenonscreen CPU/FPS/GPU/RAM
metrics. SDK432 current-modewindowedrepair andcompositedpresentation are built;
actualrelaunch pending. Userworkingdxvkspike savedbaselineAPK usesexactcurrent
SDL/DXVK providers, avoidingexclusive-modeenumeration. No providerupgrade.

## Real window, device and graphics asset loading

SDK432/APK4 exercises the measured current-display windowed fallback: real SDL
Vulkan window, Adreno610 Vulkan1.1.128 device and1920x1200 MAILBOX swapchain.
Android reports RGB565 display modes while the D3D current mode is X8R8G8B8;
exclusive enumeration remains empty. A genuine odd-address wavelet16bit read
then aborts in startup image loading.

SDK433/APK5 incorporates the byte-stream wavelet repair after30 actual decoder
cases match the normal Windows golden results. All seven packaged libraries
exactly match stage5; installation succeeds and the imported assets/receipt
remain available. Engine BuildId37a9e475a97d041bdd4697f371456b2d7e6e719e.
The real app loads graphics fastfiles beyond the failed image and initializes
render targets, static/dynamic buffers and Direct3D queries. At45s the cinematic
job aborts on its no-argument callback being called as int(jqBatch*). The exact
address0x7c8e74 resolves _UpdateFrameCallback inr_cinematic.cpp616; the matching
raw log and crash stack are retained. Initialization return and first displayed
frame remain unverified.

The user's SynchronizePresent VK_SUBOPTIMAL_KHR fix is already present in the
packaged provider, verified directly in its machine code: SUCCESS and SUBOPTIMAL
return without recreating the swapchain; error statuses retain recovery. The
current DXVK bytes equal the user's working saved spike baseline. No provider
or Vulkan version was changed and no assets were copied again.

Evidence: native_registry_closure/sdk-native-entry432-launch1-only.log,
sdk-native-entry433-launch1-only.log, sdk-native-launch433-crash1.txt,
sdk-package-verification5.json anddxvk-synchronize-present-suboptimal-disassembly.txt.

## Cinematic callback and native command-stream integration

SDK434/APK6 passes the repaired cinematic queue signature and loads ui_mp in
17.809s. The loading-screen version-text producer then accesses an eight-byte
aligned native-pointer command at a four-byte stream boundary and aborts.
The actual runtime stream repair aligns each allocated/header stride to eight
bytes, including end markers and variable text. Consumers retain the canonical
header-byteCount walk. Windows behavior and all fixed asset formats are retained.

Production Android and Windows x86/x64 whole-TU checks pass. One focused actual
HONOR fixture completes785 command allocator/text/console/list/walk/alignment/
ushort/reserve/capacity/canary checks, exit0. Two earlier fixture setup failures
are preserved: recursive harness instrumentation hooks and missing actual viewInfo
binding; only harness code was corrected. These checks provide no GPU readiness.

SDK435/APK7 builds and installs with all seven actual stage7 provider hashes
matched. Engine BuildIdcd75596a4e068bb0691f77c8ccf00142611b6e26. A native
observation logs at most three real successful Present returns for an actual
foreground window; skipped resize/screenshot paths cannot report them. Visible
content and canonical startup return still require actual observation.

Detailed contracts: native_render_command_alignment.md and
native_first_present_observation.md. SDK435 production observation is in progress;
no assets were transferred/imported again.

## Actual rendered SDK initialization return

SDK439/APK11 uses the menu-label byte cursor, native asset callback family,
filesystem allocation/page reserve and text-glow repairs. Genuine startup
finishes and the shared canonical RunNativeEngine records actualCom_Init returned
and entering the input/frame loop. It disables the initialization trace scan
only after that real return. The normal SP front-end queues mapfrontend.
This is the first proved rendered SDK initialization return, not a stub or
headless substitute. Matching engine BuildIdc1f7844eacb7ebe2d7ed2efd773fbf44eed1d7d0.

The frontend loading screen then reaches Menus_Open and UBSan rejects its
legacy cross-array top-menu expression: Menus[2*openMenuCount+599], index601
beyond the declared600 entries. The actual adjacent menuStack must be addressed
through its own typed fields. A normal completed client frame, visible game
content and gameplay remain unverified. Real loading Present returns succeeded
again, with the actual foreground window; these remain API-return evidence.

Actual evidence: native_registry_closure/sdk-native-entry439-launch1-only.log
(lines19052..55 startup return), sdk-native-launch439-crash1.txt,
sdk-package-verification11.json and stage11/native-manifest.json. No assets
were transferred/imported again and the user's DXVK SUBOPTIMAL fix remains
in the unchanged protected provider.

## Actual frontend UI progress through SDK442

SDK440 passes the typed menuStack fix; SDK441 passes the exact native vertex
color copies (777 actual HONOR outputs match Windows). SDK442 passes the
visibility mask and confirmed rectangle/float copy family. All native/Windows
checks preserve Windows bodies and canonical data/math; original retail assets
and protected DXVK/SDL providers are unchanged.

SDK442/APK14 seven packaged SOs match stage14; engine BuildId
bafb25441f54ec530624dd40f5abf90e296c3e6c. Actual Com_Init returns and enters
the input/frame loop, then real UI_DrawLoadBar aborts on its inherited cast:
double() progress getter called as double(double(*)()) with a bogus argument.
Its canonical noarg direct call is the next repair. No normal client frame,
visible game frame, rendered gameplay or HUD is proved. Actual process7976 is
absent/guarddirty; USBstayawake restored0. Observer exit0 is host observation
completion, not engine success.

Evidence native_registry_closure/sdk-native-entry442-launch1-only.log,
sdk-native-launch442-crash1.txt, sdk442-engine-maps1.txt, sdk442-screen-final1.png
and sdk-package-verification14.json. Reports native_ui_visibility_rect_alignment.md,
native_ui_top_menu_stack.md and native_vertex_color_pack_alignment.md retain
scoped ABI/equivalence/fixture details. No assets were retransferred/imported.

## First visible engine UI: SDK444

SDK443 passes the direct progress getter calls; SDK444 passes the correctly
typed bool progress callback. Genuine Com_Init returns and the input/frame
loop proceeds. The actual tablet screenshot sdk444-screen-live1.png shows
engine loading text, font rendering, borders and the default checkerboard
background while frontend.ff loads. This proves visible engine UI drawing,
not a completed frontend or playable Zombies.

SDK444 stage16 engine BuildId28df671011f1cc3bd9e4668cad970d3aac8dc77c;
all7 packaged provider bytes match and installation succeeds. The same running
process continued without relaunch, loaded frontend assets, then aborted at
cg_weapon_options.cpp121: the16x4 colors owner is flattened through its first
float[4] row. Typed row accesses are the next native repair. Process14572 is
now absent/guarddirty; USBstayawake restored0. No assets were recopied.

Evidence sdk-native-entry444-launch1-only.log, sdk-native-entry444-follow1-only.log,
sdk-native-launch444-follow1-crash.txt, sdk444-screen-live1.png and package
verification16. Source checks are in native_cgame_progress_callback_signature.md
and native_ui_loadbar_callback_signature.md. Parallel HUD preparation starts
after this first actual visible UI; GPU/CPU/FPS/RAM claims still require samples.

## Active frontend and measured overlay: SDK446–450

SDK446 reaches canonical `CA_ACTIVE` after queued resource callbacks use their
actual signatures. Subsequent launches pass the sound listener, ADS depth
buffer, FX camera and client score-order gates after narrow native repairs.
Each repair retains Windows oracle behavior and canonical calculations; the
dedicated family reports preserve source hashes and comparisons. Complete
frontend 3D rendering and playable Zombies remain unverified.

The actual APK overlay now displays measured render submission FPS/frame time,
main/render/process CPU and RSS. SDK447 also resolves real D3D GPU timestamp
queries: `sdk447-gpu-live-3.png` shows GPU span12.92ms at age0.3s. SDK448 verifies
same-process volume-up off/on and a fresh GPU sample after re-enabling. These
are loading measurements, not steady gameplay benchmarks. Source/lifetime
checks pass; pause/resume, surface loss and canonical shutdown measurement
checks still require a stable running client. See native_performance_overlay.md
and native_performance_gpu_overlay.md.

SDK450/stage22 BuildId3850c57b9fa3ab40d4ca7b47098475224bed6919 logs the new
optional startup trace policy disabled before actual initialization and still
returns from real Com_Init into the input/frame loop. Frontend becomes active
at78.161s, compared with SDK449's140.885s; this is an observed launch trend,
not a controlled paired benchmark. O0/debug/instrumentation/UBSan/assertions
and canonical startup remain intact. All seven packaged providers match their
stage manifest; installation succeeds; the existing imported assets are reused.

SDK450 then independently exposes two failures: CScr_SetFxAngles accesses past
a three-element row in the newest native-entry segment, and matching crash
PID12927/SERVERtid13085 records SIGSEGV0x30 through gjk_query and SpectatorThink.
The latter is a separate stack, not the client FX sanitizer backtrace. Both raw
observations and the matching immutable stage22 ELF remain preserved. Current
repair owners and the next build are tracked in .ledger/STATE.md.

## Client rendering integration: SDK451–461

All eleven staged launches reuse the existing imported game assets and the seven
real providers. Each package is checked against its stage manifest before the
successful installation. DXVK and SDL still match the working spike; the user’s
`VK_SUBOPTIMAL_KHR` presentation fix remains packaged. These debug launches keep
native assertions and non-recovering UBSan enabled.

Client execution now passes repaired FX axes, collision coordinate copies,
quad coordinates, key binding rows, DPVS planes, static-model LOD rows, streaming
bounds, particle-cloud axes and actor callback contracts. Family reports retain
the corresponding Windows comparisons and source proofs. Actual SDK459 then
exposes a packed snapshot scalar store; its canonical codec fixture passes380
checks on the tablet after a fixture-only owner-alignment correction. SDK460
advances to sound processing without that report. Conditional paths and the
exact packed field’s later execution are not independently claimed.

The finite SDK460 recording is preserved at
`build-android-audit/native_registry_closure/sdk460-engine-video1.mp4`.
The extracted `sdk460-postloading-frame92.png` shows the compass, build label and
measured overlay on a black scene after loading. This is actual post-loading HUD
proof; visible 3D geometry and stable gameplay remain unproved. Recording overhead
makes these samples unsuitable for a gameplay performance comparison.

SDK460 independently reports a filename cursor accessing a misaligned whole
sound struct and curve reads through the name member instead of its owned points.
The two native repairs preserve lowercasing, curve layout and L16 interpolation
precision. SDK461/stage33 BuildId `6f56e48691d11a65ac309de2b8558c2ad197de51`
returns from real Com_Init and reaches CA_ACTIVE at81589ms without those reports.
It then aborts during client prediction: the transient allocator adds112 bytes
to its legitimately null initial cursor before creating its first block.
Matching PID16859/SDLThread16932 and the immutable stage33 ELF identify
allocate→gjk_query_prims→cached_query_prolog→Pmove→CG_PredictPlayerState→
CG_DrawActiveFrame→Com_Frame. The process is dead; its dirty instance marker is
preserved. Helper50522 completed its observation and restored USB stay-awake0.

The same launch reports a nonfatal incorrect-length sound warning and the
protected provider’s unsupported stencil-export diagnostic. The sound warning
is under a bounded ZIP metadata-width audit; no damaged-asset or visual-cause
claim is made from these diagnostics. Current source owners and next integration
are maintained in `.ledger/STATE.md`. No assets were recopied.


## SDK462–468: title, input and first visible 3D room

The intervening actual APK runs integrated the transient allocator, ZIP clone
metadata, typed collision matrix, native model-surface alignment, occlusion
flag initialization and actor transition member repairs. The corresponding
source reports preserve individual production failures, bounded fixtures and
Windows comparisons. Assets and the validated DXVK/SDL providers were reused.

SDK463 reached an actual graphics submission failure and then stalled in
DXVK error recovery. Live matching-process LLDB evidence proves a recursive
attempt to acquire its nonrecursive submission mutex. The initial Vulkan
failure cause is unresolved. The engine now requests canonical r_aaSamples1;
subsequent runs pass that earlier stall before later engine failures. This is
a working MSAA-off configuration, not proof that all MSAA is unsupported.
The protected SUBOPTIMAL acceptance fix remains in the exact packaged provider.
See native_dxvk_submission_error_stall.md.

SDK465 and SDK467 display the genuine title/Press Any Key with fresh measured
FPS, CPU, GPU span and RSS. SDK467 physical Enter revealed a real UI script
callback signature mismatch. SDK468 integrates its exact native adapter and
passes Enter into the first captured 3D interrogation-room geometry and lighting.
The current stage40 engine BuildId is 16169f5dc9777d411dff30bfa34da49cabf88ae7.
All seven packaged provider bytes match their staged manifests; installation
succeeded. No original assets were modified or retransferred.

SDK468 also closes the actual input-frame call chain and verifies the rendered
input definition in common.cpp's real startup-services compile command. Its
newly active text-input path exposed the old predicate opening Android's keyboard
on every menu. A narrow canonical editing-field predicate is frozen for SDK469.
Actual touch navigation, gameplay input and lifecycle tests remain outstanding;
source fixtures alone are not tablet readiness proof.

Actual sdk468-screen-enter1.png and sdk468-screen-back1.png show room geometry,
8.3/8.4 render-submission FPS and fresh104.84/108.97ms GPU spans with CPU/RSS.
These are startup/front-end samples from an instrumented debug engine, not a
steady playable-Kino benchmark. The keyboard still obstructs the lower display
in those captures. SDK468 later aborts on bo1-render10586/PID10472 at rb_tess1177:
2147474811+15540 overflows the embedded geometry statistics accumulator through
spot-shadow drawing. The matching stage40 crash and latest native log are saved
under native_registry_closure/sdk-native-launch468-console1-crash.txt and
sdk-native-entry468-console1-only.log. The console-key sequence does not prove
console-specific causation.

PID10472 is dead and its dirty guard remains. The observation helper completed
and restored USB stay-awake0; helper success is not clean engine shutdown.
SDK469 will integrate the actual per-view accumulator reset plus the IME fix.
The goal remains active until genuine stable rendering and playable Zombies.


SDK469's real room run sustains main4601 frames without the prior statistics
overflow, then reveals a separate console-name callback mismatch. SDK470
(stage42 BuildId b1a729750397ae1b463e0dfab606091d97232638) integrates four exact
native autocomplete adapters, successfully executes genuine console queries and
continues rendering. All seven packaged providers match, no assets recopied.
PID28098 is live at handoff; do not restart simply because observation ended.

Actual menu pool queries prove main/main_text/main_setup/main_sp/main_lockout
and Zombies level menus are present. Save-device reading is false/completed,
menu-level notifications enabled, multiplayer disabled and show-only filter
empty. A bounded canonical main_text open trace reaches item visibility gates:
main_button1..5 are hidden with ui_choicegroup empty. The precise script handoff
producing that state remains unresolved; no visibility/localvar bypass is added.
Debug/list tracing are restored0 and console closed. sdk470-screen-room-clean1.png
shows genuine room geometry, fresh7.5FPS/CPU/RSS/GPU and touch UI. Main/server10700
frames were already reached without current runtime error. This is visible
rendering progress, not playable Kino or a controlled steady benchmark.
