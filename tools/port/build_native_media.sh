#!/usr/bin/env bash
set -eu
export PATH=/mingw64/bin:/usr/bin:$PATH
repo=/d/bo1-native-codec-audit
ndk=/d/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64
cd "$repo/build-android-audit/native_media"
"$repo/build-android-audit/media-source/configure" --disable-everything --disable-programs --disable-doc --disable-autodetect --enable-avformat --enable-demuxer=bink --enable-protocol=file --disable-avfilter --disable-avdevice --disable-swresample --disable-swscale --enable-avcodec --enable-avutil --enable-decoder=wmav2,bink,binkaudio_dct,binkaudio_rdft --disable-network --disable-asm --disable-gpl --disable-nonfree --disable-iconv --disable-debug --enable-small --enable-pic --enable-static --disable-shared --host-cc="/c/PROGRA~1/LLVM/bin/clang.exe --target=x86_64-w64-windows-gnu --sysroot=C:/msys64/mingw64 -fuse-ld=lld" --enable-cross-compile --arch=aarch64 --target-os=android --cc="$ndk/bin/clang.exe --target=aarch64-linux-android28 --sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot" --ar="$ndk/bin/llvm-ar.exe" --nm="$ndk/bin/llvm-nm.exe" --ranlib="$ndk/bin/llvm-ranlib.exe" --strip="$ndk/bin/llvm-strip.exe" --prefix="$repo/build-android-audit/native_media/install"
make -j8
make install
