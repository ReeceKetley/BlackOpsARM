param([string]$Ndk='D:/Android/Sdk/ndk/30.0.16248370')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $repo 'build-android-audit/native_registry_closure'
$bin=Join-Path $Ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin'
$common=@('--target=aarch64-linux-android28',"--sysroot=$Ndk/toolchains/llvm/prebuilt/windows-x86_64/sysroot",'-fsanitize=undefined','-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections','-O1','-g',"-I$repo/src")
$objects=@()
foreach($name in @('inflate','inffast','inftrees','adler32','crc32','zutil')) {
 $object="$out/wma-zlib-$name.o";$objects+=$object
 & "$bin/clang.exe" @common -c "$repo/src/zlib/$name.c" -o $object *> "$out/wma-zlib-$name.log"
 if($LASTEXITCODE){throw "zlib $name compiler failed"}
}
$args=$common+@('-std=c++20','-fms-extensions','-fsigned-char','-include',"$repo/src/platform/win32_compat/msvc_compat.h", "-I$repo/../DXVK-Native/include/native/windows", "-I$repo/build-android-audit/native_wmav2/install/include", "$repo/tools/port/tests/native_wma_decoder_tests.cpp", "$repo/src/platform/android/wma_decoder.cpp", "$repo/src/platform/fastfile_inflate.cpp")+$objects+@("$repo/build-android-audit/native_wmav2/install/lib/libavcodec.a", "$repo/build-android-audit/native_wmav2/install/lib/libavutil.a",'-lm','-latomic','-landroid','-lmediandk','-static-libstdc++','-Wl,--gc-sections','-o',"$out/native-wma-decoder-tests")
& "$bin/clang++.exe" @args *> "$out/wma-decoder-native-build1.log"
if($LASTEXITCODE){Get-Content "$out/wma-decoder-native-build1.log" -Tail 30;throw 'WMA fixture compiler failed'}
Write-Output "$out/native-wma-decoder-tests"
