# Native SDL input and matching Activity bootstrap

Frozen 2026-10-13. This batch supplies the real rendered input producer while
preserving the existing native headless console path. It does not demonstrate
an initialized APK, an Android video surface, GPU input, IME rendering, or gameplay.

## Production selection

Add `src/platform/android/rendered_input.cpp` and canonical
`src/win32/win_input.cpp` to the FULL native engine source list. Define
`BO1_NATIVE_RENDERED_INPUT=1` for `input_initialization.cpp` and
`renderer_window.cpp` (a definition on their containing target is sufficient).
The feature-off bodies retain the previous explicit rendered-input rejection;
both feature-off translation units also compile. No central CMake or diagnostic
source was edited by this batch.

Canonical `IN_GamepadsMove` and its original button mapping are unchanged. The
Win32 mouse/lifecycle bodies and Win32-only window procedure header are guarded
for Windows. Actual SDL controller samples continue through that original
client/gamepad algorithm; the adapter does not reproduce it. Windows whole-TU
comparison scope is `win_input.cpp` and `rb_backend.cpp`, both ordinary ABIs.
All modified existing files retain CP1252 encoding and the Windows branches.
Actual normal Windows whole-TU compilation PASS x86 and x64, two translation
units each, no native selectors, exit0. Results and source manifests are in
`build-android-audit/rendered-input-windows1/{x86,x64}`.

## Input and retirement contracts

Rendered initialization requires the actual successful SDL Java setup
publication. It installs a real SDL event consumer before canonical controller
initialization. Console commands become available only after this actual input
owner is installed. Missing Java setup cannot publish an event/input owner.

Keyboard events use canonical `Key_StringToKeynum` for named keys, stored
scancode-to-key ownership for releases, and per-key source counts. Both shift
keys, or a physical left button and the primary touch contact, therefore retain
the shared engine key until the last source releases it. Alt-Enter preserves
the original loading/developer/fullscreen command policy. SDL committed text
must be terminated and valid UTF-8 within the actual fixed SDL buffer. Its
unsigned bytes are delivered to the existing byte-field API; this is not a
claim of Unicode glyph or composition support. Composition preedit is not a
committed character event. Hardware backspace and Ctrl-A/C/E/V retain the
canonical character-control contract.

Mouse and touch require an exact tracked, focused, foreground, non-closing SDL
window. Logical mouse positions/deltas use actual drawable dimensions, and
frame deltas are checked against the canonical signed-int capacity. Movement
is buffered during event pumps and consumed by `CL_MouseEvent` only in the
actual input frame, after `Platform_InputFrame`; initialization wait pumps
cannot enter client mouse code early. With actual mouse focus, a frame also
reads real `SDL_GetMouseState` when there was no motion event. Client recenter
requests change actual SDL relative mouse mode and explicitly fail if the SDK
cannot apply it. Touch preserves full-width touch/finger identities and exposes
the original desktop client's single cursor/button using one primary finger.
Secondary contacts do not invent virtual gamepad controls. Matching SDK hints
disable duplicate synthesized mouse/touch streams.

Focus/background/close retirement releases held keys/buttons and pending
movement. Input shutdown also retires the actual console command producer and
controllers. The SDL event owner remains installed while a real tracked window
or renderer video reference still exists. `RetireRendererVideo` retires that
event owner only after actual window destruction and SDL video teardown.
Consequently a partial shutdown cannot abandon a live renderer's owner-dispatch
consumer. Normal renderer shutdown before input shutdown also remains valid.
These new input routines hold no mutex across engine event/client/error calls.

Native `IN_ShowSystemCursor` publishes through the existing atomic SDL cursor
request. Native backend presentation no longer rereads the plain Windows
`g_showCursor` global or overwrites input/UI requests. Windows cursor behavior
is unchanged. Actual cursor/relative-mode support on Android remains unexecuted.

## Real SDK adoption and future APK integration

`EngineActivity` extends the existing protected SDK `SDLActivity`. It calls the
SDK superclass `onCreate`, then invokes the new native adoption entry. The
controller service validates the actual UI Looper, JavaVM, Activity lifecycle,
Activity classloader, matching SDK class, non-broken-library state, exact
singleton/context identity, real surface object and real joystick handler.
Only then does it inspect the actual SDK JNI environment and Activity getters
and publish Java setup. It never repeats `SDL.setupJNI` after superclass setup.
Failed adoption refuses native engine resume/start. Duplicate attempts and
Activity recreation are not certified as recovery paths.

For a future real APK target, compile `sdl_activity_bootstrap.cpp` directly into
the engine shared library so the JNI entry is exported rather than discarded
from an unreferenced archive. Package the matching SDK Java source set and
`libSDL2.so`, select `EngineActivity` in the manifest, and supply the actual
`libblackopsarm_engine.so` shared entry exporting the SDK's expected `SDL_main`.
The current environment-only Activity/manifest/Gradle/native main remain
unchanged. Engine entry routing, actual renderer startup, application-UID asset
storage/import, and positive Activity lifecycle execution remain integration
work; shell staging is not APK storage readiness.

## Evidence and reproduction

Evidence is under `build-android-audit/native_registry_closure`:

- `rendered-input-frozen-hashes.json`: 14 source/helper/fixture hashes and sizes.
- Eight instrumented production ARM64 translation units compile: audit1 for
  controller/input-init/window/renderer-window/bootstrap/backend, audit2 for
  `win_input`, final3 for `rendered_input`. Exact argv JSON and logs are retained,
  with defined/undefined symbol manifests for every object.
- `input_initialization-rendered-input-off1` and
  `renderer_window-rendered-input-off1`: feature-off compilation succeeds.
- `rendered-input-java-build1-command.json` and log: new Activity plus the actual
  pinned SDK Java source set compiles using API35 and Java release17. The 72
  warnings concern existing SDK deprecations; no protected source was changed.
- `rendered-input-fixture-build2-command.json` and
  `rendered-input-fixture-run2.log`: real HONOR ARM64 UBSan PASS. The fixture links
  actual `cl_keys.cpp`/`q_shared.cpp` key lookup and real SDL/controller/window
  services. It checks ASCII/console/keypad/modifier/F-key mapping, invalid and
  complete UTF-8 sequences, terminated-buffer bounds, paired-source releases,
  exact focus retirement bookkeeping, signed movement accumulation/scaling,
  and three fatal-return subprocess guards. Real absent Java setup is rejected;
  wrapped unsafe JNI getters abort if accidentally entered. Scoped engine
  callbacks that would require an initialized client deliberately abort rather
  than return simulated game readiness.

Rebuild the fixture by executing the saved JSON argv from the repo root. Only
the new 2,165,568-byte fixture was staged to
`/data/local/tmp/blackopsarm-rendered-input-fixture`; existing SDL/game assets
were reused. Actual invocation:

```sh
LD_LIBRARY_PATH=/data/local/tmp/blackopsarm-native-engine-entry47-1 /data/local/tmp/blackopsarm-rendered-input-fixture
```

This fixture exercises CPU/byte/ownership contracts and negative JNI readiness.
It does not create a fabricated Activity, SDL video window, GPU device, or
initialized client, and cannot prove positive rendered interaction or recovery.
