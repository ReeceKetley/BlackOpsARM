$ErrorActionPreference='Stop'
$taskCompiler='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskOut='build-android-audit/native_game_variant_rows'
$taskFlags=@('--target=aarch64-none-linux-android28','--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot','-std=c++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all','-D_DEBUG=1','-DBO1_NATIVE_COM_BOOT=1','-DBO1_NATIVE_HEADLESS_POLICY=1','-DBO1_NATIVE_UDP_ENGINE=1','-DBO1_NATIVE_NET_ADDRESS_POLICY=1','-Isrc','-I.','-I../DXVK-Native/include/native/directx','-I../DXVK-Native/include/native/windows','-I../DXVK-Native/third_party/sdl-prefix/include/SDL2','-include','src/platform/win32_compat/msvc_compat.h')
& $taskCompiler @taskFlags tools/port/tests/native_game_variant_rows_tests.cpp src/ui_mp/ui_gametype_variants_mp.cpp '-Wl,--gc-sections' '-static-libstdc++' -o "$taskOut/native_game_variant_rows_tests" *> "$taskOut/fixture-build1.log"
if($LASTEXITCODE){throw 'actual variant row fixture build failed'}
