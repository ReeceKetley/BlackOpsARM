$ErrorActionPreference = 'Stop'
$taskCompiler = 'D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskOutput = 'build-android-audit/native_tables_asset_helpers'
New-Item -ItemType Directory -Force $taskOutput | Out-Null
$taskArguments = @(
    '--target=aarch64-none-linux-android28',
    '--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot',
    '-std=c++20', '-fms-extensions', '-fsigned-char',
    '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined',
    '-fno-sanitize-recover=all', '-static-libstdc++',
    '-Wl,--gc-sections', '-Wl,--error-limit=0', '-DBO1_STARTUP_SERVICES=1',
    '-Isrc', '-I.', '-I../DXVK-Native/include/native/directx',
    '-I../DXVK-Native/include/native/windows', '-include',
    'src/platform/win32_compat/msvc_compat.h',
    'tools/port/tests/native_tables_asset_helpers_tests.cpp',
    'src/bgame/bg_misctables.cpp', 'src/game/g_weapon_load_obj.cpp',
    'src/universal/com_encode.cpp', 'src/universal/q_parse.cpp',
    'src/platform/android/legacy_crt.cpp', '-o',
    "$taskOutput/native_tables_asset_helpers_tests"
)
& $taskCompiler @taskArguments *> "$taskOutput/build4.log"
if ($LASTEXITCODE) { Get-Content "$taskOutput/build4.log"; exit $LASTEXITCODE }
