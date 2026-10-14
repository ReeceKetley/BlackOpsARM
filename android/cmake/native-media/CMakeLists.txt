cmake_minimum_required(VERSION 3.22)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(bo1_native_media_dependency C)
endif()
# Pinned actual WMAv2/Bink codec dependency for the established Windows Android build host.
if(NOT ANDROID OR NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "Native WMAv2/Bink recipe requires the actual Android ARM64 target")
endif()
if(NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "This audited FFmpeg build recipe requires its Windows/MSYS2 host adaptation")
endif()
if(NOT ANDROID_PLATFORM_LEVEL STREQUAL "28" OR NOT ANDROID_NDK STREQUAL "D:/Android/Sdk/ndk/30.0.16248370")
    message(FATAL_ERROR "Audited WMAv2/Bink recipe requires NDK30.0.16248370/API28; extend the actual shell recipe before another toolchain")
endif()
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_program(BO1_CODEC_BASH bash.exe PATHS "C:/msys64/usr/bin" NO_DEFAULT_PATH REQUIRED)
set(BO1_WMA_OUT "${BO1_ROOT}/build-android-audit/native_media/install")
file(MAKE_DIRECTORY "${BO1_WMA_OUT}/include")
# The shell script uses the verified no-spaces alias to this exact ARM worktree;
# preparation validates or creates that junction and rejects unrelated ownership.
add_custom_command(OUTPUT "${BO1_WMA_OUT}/lib/libavcodec.a" "${BO1_WMA_OUT}/lib/libavutil.a" "${BO1_WMA_OUT}/lib/libavformat.a"
    COMMAND "${Python3_EXECUTABLE}" "${BO1_ROOT}/tools/port/prepare_native_media.py"
    COMMAND "${BO1_CODEC_BASH}" "${BO1_ROOT}/tools/port/build_native_media.sh"
    WORKING_DIRECTORY "${BO1_ROOT}"
    DEPENDS "${BO1_ROOT}/tools/port/prepare_native_media.py"
        "${BO1_ROOT}/tools/port/build_native_media.sh"
        "${BO1_ROOT}/tools/port/patches/ffmpeg-7.1.2-wmav2-eos.patch"
        "${BO1_ROOT}/third_party/ffmpeg-7.1.2/UPSTREAM.json"
        "${BO1_ROOT}/third_party/ffmpeg-7.1.2/libavcodec/wmadec.c"
    VERBATIM)
add_custom_target(bo1_native_wmav2_build DEPENDS "${BO1_WMA_OUT}/lib/libavcodec.a" "${BO1_WMA_OUT}/lib/libavutil.a" "${BO1_WMA_OUT}/lib/libavformat.a")
add_library(bo1_native_wmav2_util STATIC IMPORTED GLOBAL)
set_target_properties(bo1_native_wmav2_util PROPERTIES IMPORTED_LOCATION "${BO1_WMA_OUT}/lib/libavutil.a"
    INTERFACE_INCLUDE_DIRECTORIES "${BO1_WMA_OUT}/include")
add_library(bo1_native_wmav2_codec STATIC IMPORTED GLOBAL)
set_target_properties(bo1_native_wmav2_codec PROPERTIES IMPORTED_LOCATION "${BO1_WMA_OUT}/lib/libavcodec.a"
    INTERFACE_INCLUDE_DIRECTORIES "${BO1_WMA_OUT}/include"
    INTERFACE_LINK_LIBRARIES "bo1_native_wmav2_util;m;atomic;android;mediandk")
add_dependencies(bo1_native_wmav2_codec bo1_native_wmav2_build)
add_dependencies(bo1_native_wmav2_util bo1_native_wmav2_build)

# Existing WMAv2 consumers deliberately share the SAME actual avcodec/util runtime.
add_library(bo1_native_media_format STATIC IMPORTED GLOBAL)
set_target_properties(bo1_native_media_format PROPERTIES IMPORTED_LOCATION "${BO1_WMA_OUT}/lib/libavformat.a"
    INTERFACE_INCLUDE_DIRECTORIES "${BO1_WMA_OUT}/include"
    INTERFACE_LINK_LIBRARIES "bo1_native_wmav2_codec")
add_dependencies(bo1_native_media_format bo1_native_wmav2_build)
