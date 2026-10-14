$ErrorActionPreference='Stop'
$taskCompiler='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskOut='build-android-audit/native_hunk_tracking'
$taskFlags=@('--target=aarch64-none-linux-android28','--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot','-std=c++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all','-D_DEBUG=1','-Isrc','-I.','-I../DXVK-Native/include/native/directx','-I../DXVK-Native/include/native/windows','-I../DXVK-Native/third_party/sdl-prefix/include/SDL2','-include','src/platform/win32_compat/msvc_compat.h')
& $taskCompiler @taskFlags tools/port/tests/native_hunk_tracking_tests.cpp src/universal/mem_userhunk.cpp src/universal/mem_firstfit.cpp src/universal/mem_fixed.cpp src/qcommon/mem_track.cpp src/platform/android/hunk_services.cpp src/platform/android/virtual_memory.cpp src/platform/android/stack_trace.cpp '-Wl,--gc-sections' '-static-libstdc++' -o "$taskOut/native_hunk_tracking_tests" *> "$taskOut/fixture-build1.log"
if($LASTEXITCODE){throw 'actual hunk tracking fixture build failed'}
