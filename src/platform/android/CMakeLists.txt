cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsARMAndroid LANGUAGES C CXX)

if(NOT ANDROID OR NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "This environment target requires Android arm64-v8a and the NDK toolchain.")
endif()

# Preserve the infrastructure probe; the APK engine uses the established native
# production recipe and its canonical sources, rather than a separate engine.
add_library(blackopsarm_environment SHARED environment.cpp)
target_compile_features(blackopsarm_environment PRIVATE cxx_std_20)
target_compile_options(blackopsarm_environment PRIVATE -Wall -Wextra -Werror)
set(BO1_NATIVE_FULL_DATABASE ON CACHE BOOL "Complete APK engine" FORCE)
set(BO1_NATIVE_SDL_APP ON CACHE BOOL "Actual SDLActivity entry" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../../android/cmake/native-engine-entry"
    native-engine EXCLUDE_FROM_ALL)
