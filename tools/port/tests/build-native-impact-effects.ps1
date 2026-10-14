$ErrorActionPreference='Stop'
$taskCompiler='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskFlags=@('--target=aarch64-none-linux-android28',
 '--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot',
 '-std=c++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections',
 '-fsanitize=undefined','-fno-sanitize-recover=all','-D_DEBUG=1','-Isrc','-I.',
 '-I../DXVK-Native/include/native/directx','-I../DXVK-Native/include/native/windows',
 '-include','src/platform/win32_compat/msvc_compat.h','-Wl,--gc-sections','-static-libstdc++')
New-Item -ItemType Directory -Force build-android-audit/native_gameplay_effects_owners | Out-Null
& $taskCompiler @taskFlags tools/port/tests/native_impact_effects_tests.cpp src/universal/q_parse.cpp src/universal/surfaceflags.cpp src/platform/android/legacy_crt.cpp -o build-android-audit/native_gameplay_effects_owners/native_impact_effects_tests *> build-android-audit/native_gameplay_effects_owners/fixture-build2.log
if($LASTEXITCODE){Get-Content build-android-audit/native_gameplay_effects_owners/fixture-build2.log;exit $LASTEXITCODE}
