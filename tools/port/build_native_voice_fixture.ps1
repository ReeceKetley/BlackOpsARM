param([string]$Ndk='D:/Android/Sdk/ndk/30.0.16248370')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $repo 'build-android-audit/native_registry_closure'
$compiler="$Ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe"
$args=@('--target=aarch64-linux-android28',"--sysroot=$Ndk/toolchains/llvm/prebuilt/windows-x86_64/sysroot",'-std=c++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections','-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-include',"$repo/src/platform/win32_compat/msvc_compat.h", "-I$repo/src", "-I$repo", "-I$repo/build-android-audit/native_speex1/include", "-I$repo/third_party/faudio-25.02/include", "-I$repo/third_party/faudio-25.02/src", "-I$repo/../DXVK-Native/include/native/windows", "-I$repo/../DXVK-Native/include/native/directx", "-I$repo/../DXVK-Native/third_party/sdl-prefix/include/SDL2")
foreach($source in @('tools/port/tests/native_voice_codec_tests.cpp','src/groupvoice/encode.cpp','src/groupvoice/decode.cpp','src/groupvoice/record.cpp','src/groupvoice/play.cpp','src/groupvoice/play_dsound.cpp','src/groupvoice/record_dsound.cpp','src/win32/win_voice.cpp','src/platform/android/voice_audio.cpp','src/platform/android/faudio_bridge.cpp','src/platform/android/controller_service.cpp','src/platform/android/window_lifecycle.cpp')){$args+=(Join-Path $repo $source)}
$args+=@("$repo/build-android-audit/native_speex1/libbo1_native_speex.a", "$repo/build-android-audit/native_faudio1/faudio/libFAudio.a", "$repo/../DXVK-Native/android/app/src/main/jniLibs/arm64-v8a/libSDL2.so",'-static-libstdc++','-Wl,--gc-sections','-o',"$out/native-voice-codec-tests")
& $compiler @args *> "$out/voice-codec-native-build1.log"
if($LASTEXITCODE){Get-Content "$out/voice-codec-native-build1.log" -Tail 20;throw 'Actual voice codec fixture build failed'}
Write-Output "$out/native-voice-codec-tests"
