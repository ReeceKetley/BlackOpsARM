#!/usr/bin/env bash
set -eu
export PATH=/mingw64/bin:/usr/bin:$PATH
repo=/d/bo1-native-codec-audit
cd "$repo/build-android-audit/media-host"
"$repo/build-android-audit/media-source/configure" --disable-everything --disable-programs --disable-doc --disable-autodetect --enable-avformat --enable-demuxer=bink --enable-protocol=file --disable-avfilter --disable-avdevice --disable-swresample --disable-swscale --enable-avcodec --enable-avutil --enable-decoder=wmav2,bink,binkaudio_dct,binkaudio_rdft --disable-network --disable-asm --disable-gpl --disable-nonfree --disable-iconv --disable-debug --enable-small --enable-pic --enable-static --disable-shared --disable-w32threads --disable-pthreads --cc="/c/PROGRA~1/LLVM/bin/clang.exe --target=x86_64-w64-windows-gnu --sysroot=C:/msys64/mingw64 -fuse-ld=lld -D__USE_MINGW_ANSI_STDIO=0 -fno-builtin-sin -fno-builtin-cos" --ar=/c/PROGRA~1/LLVM/bin/llvm-ar.exe --ranlib=/c/PROGRA~1/LLVM/bin/llvm-ranlib.exe --nm=/c/PROGRA~1/LLVM/bin/llvm-nm.exe --target-os=mingw32 --arch=x86_64 --prefix="$repo/build-android-audit/media-host/install"
make -j8
make install
