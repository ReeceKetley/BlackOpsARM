param([string]$Ndk='D:/Android/Sdk/ndk/30.0.16248370')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $repo 'build-android-audit/native_registry_closure'
$compiler="$Ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe"
$args=@('--target=aarch64-linux-android28',"--sysroot=$Ndk/toolchains/llvm/prebuilt/windows-x86_64/sysroot",'-std=c++20','-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections',"-I$repo/src","$repo/tools/port/tests/native_oom_error_tests.cpp","$repo/src/platform/android/oom_error.cpp",'-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=realloc','-Wl,--wrap=sysinfo','-static-libstdc++','-Wl,--gc-sections','-o',"$out/native-oom-error-tests")
& $compiler @args *> "$out/oom-error-native-build1.log"
if($LASTEXITCODE){Get-Content "$out/oom-error-native-build1.log" -Tail 20;throw 'Actual OOM fixture compiler failed'}
Write-Output "$out/native-oom-error-tests"
