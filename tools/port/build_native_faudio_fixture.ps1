param([string]$Ndk = 'D:/Android/Sdk/ndk/30.0.16248370')
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out = Join-Path $repo 'build-android-audit/native_registry_closure'
$compiler = Join-Path $Ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$args = @('--target=aarch64-none-linux-android28', "--sysroot=$Ndk/toolchains/llvm/prebuilt/windows-x86_64/sysroot", '-D_DEBUG=1', '-DANDROID', '-std=gnu++20', '-fPIC', '-fms-extensions', '-fsigned-char', '-ffunction-sections', '-fdata-sections', '-O0', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all', '-include', "$repo/src/platform/win32_compat/msvc_compat.h", "-I$repo/src", "-I$repo", "-I$repo/../DXVK-Native/include/native/directx", "-I$repo/../DXVK-Native/include/native/windows", "-I$repo/../DXVK-Native/third_party/sdl-prefix/include/SDL2", "-I$repo/third_party/faudio-25.02/include", "-I$repo/third_party/faudio-25.02/src")
$sources = @('tools/port/tests/native_faudio_graph_tests.cpp', 'src/platform/android/faudio_bridge.cpp', 'src/platform/android/controller_service.cpp', 'src/platform/android/window_lifecycle.cpp', 'src/platform/android/profile_services.cpp', 'src/sound/snd_driver_xaudio2_dsp.cpp', 'src/sound/snd_dsp.cpp', 'src/sound/snd_radverb.cpp', 'src/universal/com_math.cpp')
$args += $sources | ForEach-Object { Join-Path $repo $_ }
$args += @("$repo/build-android-audit/native_faudio1/faudio/libFAudio.a", "$repo/../DXVK-Native/android/app/src/main/jniLibs/arm64-v8a/libSDL2.so", '-static-libstdc++', '-Wl,--gc-sections', '-o', "$out/native-faudio-graph-tests")
& $compiler @args *> "$out/faudio-graph-android-build-final.log"
if ($LASTEXITCODE -ne 0) { Get-Content "$out/faudio-graph-android-build-final.log" -Tail 30; throw "fixture compiler exited $LASTEXITCODE" }
Write-Output "$out/native-faudio-graph-tests"
