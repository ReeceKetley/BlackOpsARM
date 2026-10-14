# Native canonical renderer lifecycle integration

2026-10-13. Source integration and focused CPU/SDL event evidence only. No
actual graphics initialization, renderer registration, lost-device recovery,
retail rendered frame, Com_Init return or gameplay execution is proven here.

## Production selection

Retire BO1_NATIVE_RENDER_GLOBALS from the FULL engine's r_init.cpp compilation;
retain BO1_NATIVE_RENDER_CONFIG. Historical globals-only targets may retain the
selector. Add src/platform/android/renderer_window.cpp to the genuine startup
library using its existing protected SDL2 linkage/headers. r_init.h is unchanged.
No protected DXVK/SDL/Vulkan source or binary is modified, and no assets moved.

The original renderer lifecycle bodies now compile for ARM: registration,
capability queries, mode enumeration, window/device creation, render target and
buffer initialization, backend assets, shutdown, resizing and recovery. Their
original algorithms and publication checks remain. Actual R_InitSystems and
R_BeginRegistrationInternal publish only after the original initialization path.
This does not establish that their complete initialized consumers will succeed.

Final evidence is build-android-audit/native_registry_closure:

- r_init_full_final4.log and r_init_final4-command.json: full canonical TU, actual
  Android toolchain and original function instrumentation, compilation passes.
- r_init-final2-defined.txt / r_init-final2-undefined.txt: actual definitions and
  remaining dependencies from the same FULL compiled bodies, fixture-compatible object.
  Final4 removes unused SDL service includes from the historical GLOBALS-only
  fallback; FULL bodies/service/test behavior are unchanged.
- renderer-lifecycle-frozen-hashes.json: SHA256/bytes for r_init.cpp/h, new service
  cpp/h, and both durable fixture sources.

Root owns final production CMake/diagnostic retirement and Windows normal whole-TU
comparisons. This report does not claim those later builds have passed.

## Actual platform contract and lifecycle

Protected DXVK Native 1.9.2 maps HWND to the actual SDL_Window pointer, and
HMONITOR to actual display index plus one. Native code uses those exact mappings.
Display position/dimensions/current refresh/DPI come from SDL, not synthetic Win32
rectangles. Optional unavailable DPI is zero, explicitly capability-unknown.
Window creation uses the actual SDL Vulkan window, attaches it to the existing
window_lifecycle owner, and obtains actual logical and Vulkan drawable sizes.
Actual drawable dimensions establish native back-buffer dimensions; canonical
scene sizing remains separate. Exclusive mode requests require an actual exact
SDL display mode, and applied fullscreen flags/drawable are checked. Unsupported
hidden window requests fail. The Windows private-desktop headless-client behavior
has no Android equivalent and native window creation rejects headless execution;
no visible Activity is exposed while claiming private/headless rendering.

The service requires the actual installed SDL event owner and successful matching
SDLJavaSetupComplete publication before unsafe Android SDL JNI getters. It checks
the real JNI environment, VM and Activity's finishing/destroyed state before
video work. It takes its own actual SDL video-subsystem reference only after
successful initialization. Cleanup of an already tracked window remains possible
on its owner after Activity destruction; it does not demand a new live Activity
in order to retire resources. No Java/bootstrap/owner setter is fabricated.

D3D factory, adapter monitor/mode enumeration, CreateDevice, Reset and native
swapchain/device/factory destruction dispatch to the actual SDL owner. Calls
which need video revalidate video/Activity before entering DXVK. Release paths
can retire existing resources without reinitializing Activity/video. Native
teardown releases actual D3D owners before destroying the SDL window because
native DXVK presenter destruction can still access HWND. Failed hardware creation
retires partial render resources, implicit swapchain/device and the unregistered
created window before retrying. Query releases use their actual interface types;
the original nonnull and zero-final-reference assertions are preserved.

Android has no initialized Windows NVAPI/ATI vendor service. Native nvInitialized,
stereo handle/activation stay unavailable, and Windows-only vendor SDK calls are
guarded while the real ordinary D3D capability/device path stays intact. No vendor
initialization success or GPU count from an absent service is invented.

## Owner pumping required in the engine

Include platform/android/renderer_window.h and invoke
bo1::android::PumpRendererOwnerRequestsIfOwner() before/after native NET_Sleep,
during Sys_LoadingKeepAlive and in the actual main input/event pump. The calls
return false on a non-owner and do not install one. The real input translator
must first install the actual immutable SDL event owner via InstallSDLEventOwner
with its real event consumer after actual Java setup. Initialization waits in
R_BeginRegistration/R_CheckResizeWindow/R_Shutdown need this pump because the
backend executes renderer initialization while the controlling main thread waits.

RunRendererOwnerTask executes directly on the actual owner or queues a synchronous
request. Pending timeouts cancel/remove a request before returning, so its caller
stack is never used later. Once the owner starts a request, its caller waits until
completion, even beyond the pending timeout. The queue mutex is released before
callback execution, SDL operations or engine error/logging. Reentrant owner tasks
can execute without reacquiring that mutex. Failure error text is copied to the
calling thread's SDL error storage. Pump work is capped at 64 requests per call
so other main-owner work can progress. Transport callbacks must return normally;
engine fatal/longjmp callbacks are issued only after dispatch returns, with no
new queue lock held. Native fatal renderer paths abort if their callback returns.

## Newly exposed mode ABI/bounds corrections

Native qsort callbacks now have their actual const-void-pointer signature and
read actual typed D3DDISPLAYMODE/int entries. The Windows casts remain unchanged.
Native resolution insertion indexes the real [256][2] rows rather than indexing
past one inner array. Native refresh/resolution helpers validate input counts,
capacities and output pointers. Native closest-refresh binary search preserves
the original exact-match/lower-refresh policy while checking top/bot endpoints
before accessing them and explicitly rejecting missing resolution/dimensions.
Mode enumeration rejects invalid actual dimensions/refresh values and bounds all
mode-label writes by the real 5120-byte storage. Windows original bodies remain.
CP1252 roundtrip is exact; the source retains all three non-ASCII bytes.

## Proportionate actual tests

Durable sources: tools/port/renderer_owner_fixture.cpp and
 tools/port/renderer_modes_fixture.cpp. Saved JSON argv commands reproduce builds:
renderer-owner-build1-command.json (last build5) and
renderer-modes-build2-command.json (last build2). The saved command fixture paths
point to identical retained copies under native_registry_closure. The modes
fixture links the actual full r_init object with section GC; its isolated log
provider is silent and its fatal provider deliberately returns, testing the native
abort fallback. It does not substitute any production renderer algorithm.

HONOR device AKWJ9X5116G04618, UBSan enabled with no recovery:

- renderer-owner-build5.log / renderer-owner-run5.log PASS. Actual SDL events and
  installed counting consumer: queued timeout cancellation/no late access, 120
  concurrent requests from four producers, reentrant owner execution, started
  request lifetime beyond its pending timeout, actual callback failure copied
  across thread-local errors, genuine SDL event delivery and owner retirement.
  Matching Java setup remains actually false in the shell. Wrapped unsafe JNI
  getters abort if touched; actual PrepareRendererVideo returns failure without
  touching either and without initializing video. This is event/dispatch proof,
  not an engine input consumer, Java bootstrap or GPU-positive proof.
- renderer-modes-build2.log / renderer-modes-run2.log PASS. Actual canonical helper
  code fills all 256 resolution rows with canary intact, sorted/deduplicated
  refresh table, lower/exact/upper refresh search boundaries, missing-mode and
  invalid/full table fatal rejection. Four child processes receive SIGABRT when
  the real native fatal path's isolated callback unexpectedly returns.

Only changed fixture binaries were transferred. The retained matching SDL library
was reused from /data/local/tmp/blackopsarm-native-engine-entry47-1. Execute:

    adb -s AKWJ9X5116G04618 shell /data/local/tmp/renderer_modes_fixture
    adb -s AKWJ9X5116G04618 shell 'LD_LIBRARY_PATH=/data/local/tmp/blackopsarm-native-engine-entry47-1 /data/local/tmp/renderer_owner_fixture'

Earlier genuine failures remain preserved: owner build1 lacked the standalone
main macro undefinition; shell run1 lacked executable mode; shell run2 showed
SDL's actual missing SDL_SetMainReady contract. The standalone fixture now calls
that real SDL entry API; it does not publish Java/video readiness.

## Remaining real prerequisites

The current APK does not run the matching SDL Java setup, and rendered engine
input has no complete actual keyboard/mouse/touch consumer. Those remain explicit
prerequisites. No APK installation or fabricated Activity was used for these
negative shell tests. DXVK Present/surface recreation and other window-sensitive
calls outside r_init still require an actual owner-dispatch audit/integration
before positive rendering. Device creation/resources/registration/resize/recovery
and initialized shutdown have not run on a real GPU, and no fixture implies that
those real consumers are ready. Continue actual full production integration.
