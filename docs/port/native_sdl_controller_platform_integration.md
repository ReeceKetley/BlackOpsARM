# Native SDL controller platform owner

The complete canonical `src/win32/win_gamepad.cpp` is adapted to use
`src/platform/android/controller_service.{h,cpp}` on Android. Root integration
owns production source selection and normal Windows comparison. Native
acquisition replaces XInput; original dvar registration, digital/analog filtering,
radial deadzone normalization, hysteresis and pressed/released algorithms remain
the engine owners. Windows retains XInput and its original behavior.

## Ownership and native behavior

The service requires the real synchronized `IsSDLEventOwner()` from the existing
window lifecycle provider. Initialization, enumeration, polling, sample access,
rumble and shutdown run on that SDL event/window owner. It opens actual SDL
GameControllers, retains device instance identities, closes detached handles,
reconciles hotplug and assigns the canonical **one controller slot**. Additional
mapped devices remain unassigned until that slot becomes empty; unmapped
joysticks are not represented as gamepads. It does not dequeue the general SDL
event queue. The established event owner must pump events before refresh.

SDL button mappings preserve the engine's original digital masks. Stick Y is
inverted from SDL down-positive to engine up-positive, with explicit -32768
handling. Trigger values are converted from SDL's nonnegative controller units
to the original eight-bit engine sample. Only attached samples commit output.
An enumerated empty slot returns explicit absence while preserving the caller's
sample; invalid ports, uninitialized service and wrong-owner access return errors.
Canonical native queries escalate uninitialized/wrong-owner access to actual
engine fatal failure rather than reporting neutral readiness. Port and analog/
stick index checks remain active outside Debug builds.

Generation changes retire old canonical state and deliver removal/insertion
callbacks, including replacement within a refresh. Native shutdown resets engine
state, closes SDL handles and releases the acquired controller subsystem before
the event owner retires. Rumble is an actual SDL call and reports unavailable or
unsupported devices instead of assuming success.

`GPad_GetStick` previously indexed its four-value array with the full tagged
`GamePadStick` enum (0x40000000+index). The native branch masks and bounds-checks
that index. Native GamePad/sample layouts are asserted (84 and12 bytes); Windows
capabilities remain in the Windows layout only. The narrow
`BO1_NATIVE_GAMEPAD_HOST_TEST` selects exactly these native engine branches for
the focused real-SDL host fixture; it is not a production exclusion or readiness
replacement. CP1252 source bytes are preserved.

Configured dedicated/headless `GPad_InitAll` retains all15 real dvar registrations
and engine state initialization, then returns before physical controller access
or rumble. It does not initialize the controller service; general queries still
fail if someone consumes them without a real rendered SDL lifecycle. Root's
separate input lifecycle must honor this mode policy.

## Android Java prerequisite

The matching SDL2 Android joystick backend invokes Java input enumeration during
initialization. Its JNI getter reads SDL's pthread TLS key, and its polling code
dereferences SDLControllerManager method IDs. A nonnull environment alone does
not prove those are initialized and can be unsafe before Java setup.

`SetupSDLControllerJava(JNIEnv*)` must be called once from an actual Java JNI
startup entry on the Android UI thread, **before SDL.setupJNI runs elsewhere**.
It verifies actual current/main Android Loopers, obtains matching SDL Java classes
and callback/handler signatures, invokes real `SDL.setupJNI()` (Activity, Audio
and ControllerManager native setup), invokes actual controller Java
`initialize()`, and verifies the live handler and SDL environment only after the
real TLS setup. JNI exceptions/failures never publish success. Once native setup
has been attempted, duplicate setup is rejected to prevent overwriting native
SDL global references/mutexes. Only fully successful real setup publishes the
acquire/release prerequisite checked by controller initialization. No token setter,
synthetic JNIEnv or shell setup fallback exists.

The current APK JNI shell does not provide this lifecycle, so native ADB shell
controller initialization fails explicitly before either unsafe SDL JNI access
or physical enumeration. Real Activity/context, matched SDL Java/native library
loading, lifecycle teardown and input delivery still need root integration. Java
setup's positive path has compilation/source audit only; it has not executed.

## Evidence

Logs are under `build-android-audit/native_registry_closure/`:

- `win_gamepad-native-compile2.log`, `controller_service-native-compile2.log`:
  final original/native production TUs compile ARM64/API28 with actual selected
  headers/options and UBSan.
- `controller-service-native-fixture-build1.log`,
  `controller-service-honor-run1.log`: actual HONOR AKWJ9X5116G04618 UBSan run
  exits0. It verifies missing event owner, null actual JNI setup, absent completed
  real Java setup and uninitialized sampling fail without output mutation or
  fabricated neutral readiness. Positive controller enumeration is not claimed.
  Only the changed fixture executable was pushed/chmod700; staged libSDL2.so was
  reused via `/data/local/tmp/blackopsarm-native-engine-entry47-1`.
- `controller-sdl-host-configure.log`, `controller-sdl-host-build.log`: actual
  protected SDL2.30.11 source built read-only into the separate Android worktree
  audit directory `build-android-audit/controller-sdl-host`, MSVC x64 static SDL.
- `controller-canonical-host-build5.log`, `controller-canonical-host-run3.log`:
  real Windows-host SDL virtual controllers and the complete canonical GPad and
  com_math TUs pass. Checks cover real enumeration/one-slot assignment, mapped
  buttons/axes/endpoints/triggers, rumble callback delivery, extra-device exclusion,
  hotplug close/reassignment, wrong-owner/invalid-port rejection, shutdown/restart,
  and original canonical deadzone/masked-stick/button/trigger/pressed/released/
  removal processing. Configured headless registration avoids controller access.
  Dvar values are stored by isolated fixture registration providers; unrelated
  unused math consumers abort if accidentally reached. No platform readiness or
  controller state is substituted. The host fixture is not ARM or APK proof.

The first host fixture failures assumed virtual trigger publication before SDL
controller binding survived opening. Real SDL binding initializes trigger state;
the fixture now opens/binds before publishing changes. Earlier failures and build
dependency logs are preserved. `controller-canonical-host-build.cmd` records the
exact final host build invocation (clang-cl/lld, actual SDL static library and
complete original math/GPad sources). Native fixture binary SHA256:
`a36fc2a70a79c99540dd7857fba1452ebb67b0969f29e7957ec38c79ff903374`.

## Remaining scope

Root must select `win_gamepad.cpp` and `controller_service.cpp`, preserve the SDL
include/library and actual event owner, and remove competing diagnostics only
where the genuine owners now replace them. SDL Java/native setup, owner installation,
event pumping, gamepad refresh, engine input translation and shutdown ordering need
actual production lifecycle binding. Physical Android controllers, their Android
hotplug/rumble, positive Java setup/exception injection, failure injection during
open/allocation, delayed debug mode, arbitrary simultaneous lifecycle callers,
general SDL keyboard/mouse/touch input, complete Com_Init/Com_Frame and gameplay
remain unverified. No APK/controller-ready claim follows from these scoped tests.

## Reproduction and Java lifecycle gate

The protected source and Android runtime are SDL2.30.11. Configure/build the
host SDL target in the worktree audit directory with the actual MSVC environment:

```bat
call "C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64
"C:/Program Files/CMake/bin/cmake.exe" -S "D:/Desktop/Black Ops 2 Android/DXVK-Native/third_party/SDL" -B "D:/Desktop/Black Ops 2 Android/BlackOpsARM/build-android-audit/controller-sdl-host" -G Ninja -DCMAKE_MAKE_PROGRAM=D:/Android/Sdk/cmake/3.22.1/bin/ninja.exe -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST=OFF -DCMAKE_BUILD_TYPE=Release
"C:/Program Files/CMake/bin/cmake.exe" --build "D:/Desktop/Black Ops 2 Android/BlackOpsARM/build-android-audit/controller-sdl-host" --target SDL2-static -j 8
```

The exact final canonical-host build command, preserved durably here:

```bat
"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64 >nul && "C:/Program Files/LLVM/bin/clang-cl.exe" -fuse-ld=lld /Zc:inline /Gw /nologo /DBO1_NATIVE_GAMEPAD_HOST_TEST /DBO1_PORT_X64=1 /D_DEBUG=1 /Gy /I"D:\Desktop\Black Ops 2 Android\BlackOpsARM" /I"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include" /std:c++20 /EHsc /MDd /O1 /DSDL_STATIC_LIB /I"D:\Desktop\Black Ops 2 Android\BlackOpsARM\src" /I"D:\Desktop\Black Ops 2 Android\BlackOpsARM\..\DXVK-Native\third_party\SDL\include" "D:\Desktop\Black Ops 2 Android\BlackOpsARM\tools\port\tests\native_controller_service_tests.cpp" "D:\Desktop\Black Ops 2 Android\BlackOpsARM\src\platform\android\controller_service.cpp" "D:\Desktop\Black Ops 2 Android\BlackOpsARM\src\platform\android\window_lifecycle.cpp" "D:\Desktop\Black Ops 2 Android\BlackOpsARM\src\win32\win_gamepad.cpp" "D:\Desktop\Black Ops 2 Android\BlackOpsARM\src\universal\com_math.cpp" /Fo"D:/Desktop/Black Ops 2 Android/BlackOpsARM/build-android-audit/native_registry_closure/" /Fe"D:\Desktop\Black Ops 2 Android\BlackOpsARM\build-android-audit\native_registry_closure\controller-service-host-tests.exe" /link /OPT:REF /NODEFAULTLIB:MSVCRT "D:\Desktop\Black Ops 2 Android\BlackOpsARM\build-android-audit\controller-sdl-host\SDL2-static.lib" kernel32.lib user32.lib gdi32.lib winmm.lib imm32.lib ole32.lib oleaut32.lib version.lib uuid.lib advapi32.lib setupapi.lib shell32.lib dinput8.lib
```

Execute `build-android-audit/native_registry_closure/controller-service-host-tests.exe`.
The final host executable SHA256 is
`f2aeab1adb6e4fad8980eec271ff63dd1d3bea8dfab9d1aabd6b0be4ad362ce3`.

Root verification after source freeze: normal win_gamepad Windows x86/x64 compile
passes; full-engine-link381 compiles selected provider/originals and reaches
remaining real links. Neither is full engine runtime proof.

**Activity teardown/recreation is not certified.** The current Java setup record
is retained for the process's initial actual setup. No teardown/reinitialization
API is implemented here. Before supporting Activity teardown/recreation, the root
lifecycle must shut down controllers and retire/reset Java ownership in concert
with actual SDL Java/native reference teardown. It must prevent setup readiness
from outliving those references and define a verified fresh setup path. Recalling
setup now is rejected, rather than guessing that the prior native global refs
remain valid. Until that lifecycle exists and executes, this implementation is
scoped to one successful Java startup lifetime (whose positive Android path is
still unexecuted).
