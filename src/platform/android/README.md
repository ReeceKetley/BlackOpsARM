# Android platform implementation boundary

This directory will hold Android implementations of the existing interfaces in
`../IPlatform.hpp`, `../IFileSystem.hpp`, `../IInputBackend.hpp`,
`../IAudioBackend.hpp`, `../IRenderBackend.hpp` and related platform headers.
The current registry and Windows adapters remain unchanged.

`CMakeLists.txt` and `environment.cpp` only verify the NDK ARM64/JNI toolchain and
APK packaging. They do not initialize the engine or implement platform services.
`narrow32_android.cpp` is the separate Phase 1 handle backend: native pointer
addresses encode into four-byte region-tagged slots using acquire/release atomic
publication and serialized registration. Region exhaustion aborts explicitly.
The standalone audit project links/tests this implementation; the environment
APK remains unchanged. See the repository README for the current port status.
Future lifecycle, SDL2/Vulkan/DXVK WSI, audio and Xbox controller input belong
here, separate from shared gameplay.
