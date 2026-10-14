# Canonical gamepad engine owners

`src/client/cl_gamepad.cpp` and `src/cgame/cg_gamepad.cpp` compile as complete
original translation units for Android. Root integration selects them and retires
the competing FULL startup diagnostic definitions. No gamepad behavior body is
excluded. `CL_InitGamepadCommands` registers actual bindaxis/unbindallaxis and
stick/button configuration commands; `CL_InitGamepadAxisBindings` initializes
the real client mappings and axis values. Original event/button/menu/demo/spectator/
killcam and axis algorithms remain the canonical owners.

The client source imports XInput-containing `win_gamepad.h` only on Windows.
Its two platform stick-state query declarations now also live next to the
existing `GPad_GetButton` declaration in the engine's `cl_gamepad.h`. This does
not implement or pretend to own controller state. Mapping/axis storage contains
only fixed-width integer enums/values; native assertions retain eight-byte
`GamepadVirtualAxisMapping` and 72-byte `GpadAxesGlob` layouts. No runtime-pointer
truncation or fixed byte stride requiring adaptation was found in these TUs.

## Native binding repair

Original `Axis_Bind_f` accepts `bindaxis <physical>` and supplies virtual axis
NONE (-1) to `Gamepad_BindAxis`. The decompiled body indexed `virtualAxes[-1]`,
overwriting preceding physical-axis values. Native binding handles that intended
unbind operation by removing all virtual mappings to the supplied physical axis,
retaining unrelated mappings and physical values, and marking bindings modified.
Native pointer, physical/virtual enum and map enum assertions reject invalid
arguments; existing assertions are not weakened. Windows retains its original
binding body. Changes were made as ASCII byte replacements, preserving source
CP1252 bytes.

## Evidence

Logs are in `build-android-audit/native_registry_closure/`:

- `cl_gamepad-native-compile1.log`: original source compilation exposes the
  unavailable XInput include dependency. `cg_gamepad-native-compile1.log` passes.
- `cl_gamepad-native-compile3.log` and `cg_gamepad-native-compile3.log`: final
  complete original ARM64/API28 TUs pass with actual production target options,
  no BO1_STARTUP_SERVICES or BO1_STRING_POOL_GATE, and UBSan.
- `cl_gamepad-fixture-compile.log`, `gamepad-axes-fixture-build1.log`:
  canonical fixture object and final NDK30.0.16248370/API28 fixture build pass.
  Only function instrumentation is omitted in the fixture object; all selected
  engine code and UBSan are retained.
- `gamepad-axes-honor-run1.log`: actual HONOR AKWJ9X5116G04618 execution exits0.
  `tools/port/tests/native_gamepad_axes_tests.cpp` checks real initialization,
  linear/radial-stick/trigger-square mappings, devgui suppression, physical unbind
  with retained values/unrelated mappings, clear-all and four child processes
  that fail explicit pointer/enum assertions. Isolated devgui state/assertion
  providers support this engine algorithm fixture; no SDL/controller input or
  readiness is fabricated. Only the changed fixture binary was transferred to
  `/data/local/tmp/blackopsarm-native-gamepad-axes-tests`, chmod700.

## Remaining dependencies and limits

Actual `win_gamepad.cpp` owns `GPad_GetButton`, `GPad_IsStickPressed`,
`GPad_IsStickReleased`, state refresh and gamepad dvar registration (including
`gpad_enabled`, menu scroll delays and controller configuration). Native device
enumeration, SDL translation, state/deadzone publication, rumble and lifecycle
must adapt that actual ownership and XInput boundary before platform input is
complete. The new engine files intentionally leave those real dependencies
unresolved until a native owner exists.

Client input additionally calls real demo input, scoreboard, mute-error input,
unlockables, UI, command/dvar/filesystem and key owners. Cgame input requires
real client-input button/view-angle helpers, cgame globals, third-person/spectator
state, demo playback and UI animation. Compiler success is not source-closure
or initialized-consumer proof. Command registration/execution with real config
assets, platform gamepad events, spectator/menu/gameplay consumers, full
Com_Init/Com_Frame and arbitrary concurrent input remain unverified.
