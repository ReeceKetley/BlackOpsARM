$ErrorActionPreference='Stop'
$taskCompiler='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskOut='build-android-audit/native_assertion_services'
New-Item -ItemType Directory -Force $taskOut | Out-Null
$taskFlags=@('--target=aarch64-none-linux-android28',
 '--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot',
 '-std=c++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections',
 '-fsanitize=undefined','-fno-sanitize-recover=all','-D_DEBUG=1','-Isrc','-I.',
 '-include','src/platform/win32_compat/msvc_compat.h','-Wl,--gc-sections','-static-libstdc++')
& $taskCompiler @taskFlags tools/port/tests/native_assertion_services_tests.cpp src/platform/android/assertion_services.cpp src/platform/android/stack_trace.cpp src/platform/android/fatal_error.cpp -o "$taskOut/native_assertion_services_tests" *> "$taskOut/fixture-build1.log"
if($LASTEXITCODE){Get-Content "$taskOut/fixture-build1.log";exit $LASTEXITCODE}
