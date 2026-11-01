$ErrorActionPreference = 'Stop'
python -I tools/port/tests/script_isalive_regression.py
if ($LASTEXITCODE) { throw 'Fixture generation failed' }
$taskCompiler = 'D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskOutput = 'build-android-audit/script-isalive-regression'
$taskFlags = @('--target=aarch64-none-linux-android28',
    '--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot',
    '-O2', '-std=c++20', '-fms-extensions', '-fsigned-char', '-ffunction-sections', '-fdata-sections',
    '-fsanitize=undefined', '-fno-sanitize-recover=all', '-D_DEBUG=1', '-Isrc', '-I.',
    '-I../DXVK-Native/include/native/directx', '-I../DXVK-Native/include/native/windows',
    '-I../DXVK-Native/third_party/sdl-prefix/include/SDL2', '-include', 'src/platform/win32_compat/msvc_compat.h')
& $taskCompiler @taskFlags "$taskOutput/fixture.cpp" '-Wl,--gc-sections' '-static-libstdc++' -o "$taskOutput/fixture" *> "$taskOutput/build.log"
if ($LASTEXITCODE) { throw 'Actual builtin regression fixture build failed' }
