$ErrorActionPreference='Stop'
$taskCompiler='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskFlags=@('--target=aarch64-none-linux-android28',
 '--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot',
 '-std=c++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections',
 '-fsanitize=undefined','-fno-sanitize-recover=all','-D_DEBUG=1','-Isrc','-I.',
 '-I../DXVK-Native/include/native/directx','-I../DXVK-Native/include/native/windows',
 '-include','src/platform/win32_compat/msvc_compat.h','-Wl,--gc-sections','-static-libstdc++')
New-Item -ItemType Directory -Force build-android-audit/native_script_persistence | Out-Null
& $taskCompiler @taskFlags tools/port/tests/native_script_persistence_tests.cpp src/universal/memfile.cpp src/minilzo/minilzo.cpp src/platform/android/narrow32_android.cpp src/universal/mem_largelocal.cpp -o build-android-audit/native_script_persistence/native_script_persistence_tests *> build-android-audit/native_script_persistence/fixture-build2.log
if($LASTEXITCODE){Get-Content build-android-audit/native_script_persistence/fixture-build2.log;exit $LASTEXITCODE}
