param([string]$Ndk='D:/Android/Sdk/ndk/30.0.16248370')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $repo 'build-android-audit/native_registry_closure'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$compiler="$Ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe"
$arguments=@('--target=aarch64-linux-android28',"--sysroot=$Ndk/toolchains/llvm/prebuilt/windows-x86_64/sysroot",'-std=c++20','-O1','-g','-D_DEBUG=1','-fms-extensions','-fsigned-char','-fsanitize=undefined','-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections',"-I$repo/src","-I$repo","-I$repo/../DXVK-Native/include/native/directx","-I$repo/../DXVK-Native/include/native/windows","-I$repo/../DXVK-Native/third_party/sdl-prefix/include/SDL2",'-include',"$repo/src/platform/win32_compat/msvc_compat.h","$repo/tools/port/tests/native_physics_arena_init_tests.cpp","$repo/src/physics/phys_mem_new.cpp","$repo/src/platform/android/oom_error.cpp",'-static-libstdc++','-Wl,--gc-sections','-o',"$out/physics-arena-init-tests")
& $compiler @arguments *> "$out/physics-arena-init-tests-build.log"
if($LASTEXITCODE){Get-Content "$out/physics-arena-init-tests-build.log" -Tail 30;throw 'Actual physics arena fixture compiler failed'}
Write-Output "$out/physics-arena-init-tests"
