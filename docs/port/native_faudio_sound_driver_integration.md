# Native FAudio canonical sound-driver integration

2026-10-13. Production source batch frozen; integration and Windows comparison owned by root. No Android audio-device or complete SD_Init success is claimed.

## Implementation

The original SD driver remains the owner of its 74 source voices, ten stream slots, two borrowed streaming windows, channel parameters, four-channel Radverb, world/no-voice bus, master bus, meters, dynamics, EQ, recording callback and device selection. Android uses a typed forwarding bridge to actual pinned FAudio and actual FAPO dispatch, rather than a replacement mixer. Windows retains its original XAudio2/XAPO branch. snd_radverb.cpp/.h are unchanged originals.

The bridge owns actual SDK voices and callbacks, stops processing during graph construction, starts only after the original graph is present, retires voices through FAudioVoice_DestroyVoiceSafeEXT before releasing callback owners, and rejects premature engine/effect retirement. FAPO parameter updates and processing share a mutex. The driver propagates construction/submission errors and checks the live SDL audio device status and SDK error latch. A retained pointer is not treated as device readiness. Actual SDL event ownership and published completion of the actual SDL.setupJNI/controller Java setup are required before Android backend access. The new SDLJavaSetupComplete accessor only reads that real setup result.

Native source buffers have naturally aligned pointer slots for atomic callback retirement; metadata is converted into FAudio's packed C buffer rather than aliasing it. Pointer publication uses release and callback exchange uses acquire/release. PCM16 headers and RAM frame bounds are repaired for the native ABI. Native source DSP accepts its actual typed parameter structure; its low-pass coefficient uses an actual float rather than overwriting a pointer through a float cast. Native effect storage supports a validated maximum of 1024 frames, with every actual valid frame processed. Windows retains its 480-frame contract. The actual upstream host backend used 480-frame processing in the graph proof. DSP512 and guard canaries are separately checked on ARM64; device-selected Android quantum/topology remains to be exercised.

## Formats and remaining codec dependency

Canonical assets describe PCM16/24/32/IEEE float, XMA, MP3, MSADPCM and WMA. The original actual driver path implements PCM16, MSADPCM (262 bytes per channel, 512 samples, seven coefficients), and nonstream WMA with seek packets; the other original formats already fail. This native batch implements the real PCM16/MSADPCM driver paths. The bridge additionally validates FAudio PCM/float metadata, whole frames, play/loop ranges, compressed header lengths and MSADPCM predictor bounds.

FAudio25.02's SDL backend has no real WMA decoder and its upstream error decoder produces silence. Native creation rejects WMA and extensible WMA before that fallback, and the canonical driver reports unsupported asset format rather than pretending to play. This is an explicit unresolved functional loss versus Windows, not a complete retail-audio implementation. Required retail WMA must gain a real matching decoder, packet/seek handling and borrowed-buffer lifetime proof before the goal can be called complete. XMA/MP3 support is not newly claimed.

## Exact dependency and integration recipe

Untouched upstream include/src/cmake/CMakeLists/LICENSE/README are vendored in third_party/faudio-25.02. UPSTREAM.json records official release25.02, commit af74e661c1bd8b105840d14485cc01d9c782b513, archive SHA256 04f66e0cfc9cfed61f4f19fd224501d19b796b0b44d2d5696d6751d2b137881f. License is retained. Primary references: https://github.com/FNA-XNA/FAudio/tree/25.02 and its README/CMakeLists/include/FAPO.h. This version supports existing SDL2; SDL/DXVK are unchanged.

Before add_subdirectory(third_party/faudio-25.02), set BUILD_SHARED_LIBS=OFF, BUILD_UTILS=OFF, BUILD_TESTS=OFF, BUILD_SDL3=OFF, XNASONG=OFF, FORCE_ENABLE_DEBUGCONFIGURATION=ON, and existing SDL2_INCLUDE_DIRS/SDL2_LIBRARIES. Set FAudio-static POSITION_INDEPENDENT_CODE ON. FORCE_ENABLE_DEBUGCONFIGURATION is mandatory because live-device observation uses the pinned FAudio_internal.h layout; the bridge rejects the incompatible disabled-debug configuration. Add both vendor/include and vendor/src to the actual native engine state target. Link final executable FAudio-static plus existing SDL2. The object target alone cannot carry the final static-library dependency.

Select these actual production sources:
- src/platform/android/faudio_bridge.cpp
- src/sound/snd_driver_xaudio2.cpp
- src/sound/snd_driver_xaudio2_dsp.cpp
- src/sound/snd_dsp.cpp
- src/sound/snd_radverb.cpp

Existing controller_service.cpp and window_lifecycle.cpp provide the actual Java/setup and synchronized event-owner prerequisites. No production test flag is required. BO1_NATIVE_FAUDIO_HOST_TEST selects only the native adapter when testing on Windows; normal Windows compilation uses its original branch.

Standalone internal dependency build:
```
D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe -S android/cmake/native-faudio -B build-android-audit/native_faudio1 -G Ninja -DCMAKE_MAKE_PROGRAM=D:/Android/Sdk/cmake/3.22.1/bin/ninja.exe -DCMAKE_TOOLCHAIN_FILE=D:/Android/Sdk/ndk/30.0.16248370/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=28 -DCMAKE_BUILD_TYPE=Debug
D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe --build build-android-audit/native_faudio1 --target FAudio-static
./tools/port/build_native_faudio_fixture.ps1
```
The fixture script provides the exact actual native source/link recipe, ARM64 UBSan, static libc++, existing SDL2 and linker section retirement. It does not link SD driver's unrelated global consumers. Full production integration must validate those separately.

## Evidence

All six native TUs (bridge, controller accessor, original driver, DSP wrapper, DSP CPU code and Radverb) compiled with NDK30.0.16248370/API28, original native compiler settings, no startup exclusion, and UBSan. Logs: build-android-audit/native_registry_closure/*-faudio-native-final.log. Pinned actual static dependency configured/built: faudio-native-configure-final.log and faudio-native-build-final.log.

Host fixture native_faudio_graph_tests.cpp linked actual FAudio25.02 and existing SDL2.30.11 with SDL_AUDIODRIVER=dummy as an explicitly selected null sink. Before that sink, actual SDK callbacks capture float output through the original source/world/master/Radverb graph. faudio-graph-host-build4.log and faudio-graph-host-run4.log pass twice: finite/nonzero graph output, PCM sample progress, pause freeze, pitch resume, two borrowed packet completions, canonical EOS counter reset, actual MSADPCM512 decoded value1000, invalid predictor rejection, WMA refusal, active looping-voice destruction followed by stable callback counters, reversed graph retirement and effect refs returned to1. This proves software mixing and ownership, not physical output. The actual driver CPP was separately compiled rather than executed in this isolated graph fixture. Host executable reproduction command is preserved in faudio-graph-host-build.cmd; it uses clang-cl/MSVC x64, actual full com_math.cpp, native bridge/controller/window and original three DSP TUs, FAudio.lib and SDL2-static.lib, standard SDL Windows system libraries.

HONOR AKWJ9X5116G04618 runs the final ARM64 fixture under UBSan: faudio-graph-android-build-final.log and faudio-graph-honor-final2.log pass. It processes actual original source DSP512 with typed LPF, finite values/canaries, aligned two-slot pointer retirement, then verifies audio factory refuses missing actual event owner and missing actual Java setup before opening a device. No assets were sent. Existing SDL2 was reused at /data/local/tmp/blackopsarm-native-engine-entry47-1. Device reproduction:
```
adb -s AKWJ9X5116G04618 push build-android-audit/native_registry_closure/native-faudio-graph-tests /data/local/tmp/blackopsarm-native-engine-entry47-1/native-faudio-graph-tests
adb -s AKWJ9X5116G04618 shell 'chmod 755 /data/local/tmp/blackopsarm-native-engine-entry47-1/native-faudio-graph-tests && cd /data/local/tmp/blackopsarm-native-engine-entry47-1 && LD_LIBRARY_PATH=. ./native-faudio-graph-tests'
```
Preserved failed logs: host runs1/2 expected an EOS counter9600 whereas the actual SDK canonically resets it to0; fixture expectation was corrected. honor-final.log records exit126 because adb push reset executable permissions; chmod then final2 passes. No engine assertion was weakened.

## Lifecycle and proof limits

Actual Java setup positive execution, physical Android output, Android audio callback rate/channel layout, Activity teardown/recreation, stream-cache integration, sound-worker scheduling and full SD_Init/Com_Init are unexecuted. Root must retire/reset Java setup ownership on actual Activity teardown before reinitialization so setup state cannot outlive Java references. Android headless input policy remains with the established platform lifecycle; no dummy Java readiness or independent setup path is introduced here. WMA remains a real codec blocker. No APK/gameplay-ready claim follows from these fixtures.
