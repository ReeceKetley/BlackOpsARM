$ErrorActionPreference='Stop'
$taskCompiler='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskOut='build-android-audit/native_sp_frame_owners'
$taskFlags=@('--target=aarch64-none-linux-android28','--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot','-std=c++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all','-D_DEBUG=1','-Isrc','-I.','-I../DXVK-Native/include/native/directx','-I../DXVK-Native/include/native/windows','-I../DXVK-Native/third_party/sdl-prefix/include/SDL2','-include','src/platform/win32_compat/msvc_compat.h')
foreach($taskName in @('g_sp_playtrace','g_sp_headless_move','g_sp_headless_throw','g_sp_headless_dice','g_sp_headless_replay')) {
    & $taskCompiler @taskFlags -c "src/game_sp/$taskName.cpp" -o "$taskOut/$taskName.o" *> "$taskOut/$taskName-final-build.log"
    if($LASTEXITCODE){throw "$taskName compile failed"}
}
& $taskCompiler @taskFlags tools/port/tests/native_sp_trace_tests.cpp src/platform/android/stdio_services.cpp src/platform/android/file_services.cpp src/platform/android/legacy_crt.cpp src/platform/win32_compat/win32_errors.cpp src/universal/q_shared.cpp '-Wl,--gc-sections' '-static-libstdc++' -o "$taskOut/native_sp_trace_tests" *> "$taskOut/trace-fixture-build.log"
if($LASTEXITCODE){throw 'trace fixture build failed'}