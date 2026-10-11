cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsARMEnvironment LANGUAGES CXX)

if(NOT ANDROID OR NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "This environment target requires Android arm64-v8a and the NDK toolchain.")
endif()

# Infrastructure probe only. The existing root engine CMake target remains the
# source of truth; its MSVC/Win32 dependencies must be isolated in later work.
# Do not copy cmake_files.cmake or engine sources into this directory.
add_library(blackopsarm_environment SHARED environment.cpp)
target_compile_features(blackopsarm_environment PRIVATE cxx_std_20)
target_compile_options(blackopsarm_environment PRIVATE -Wall -Wextra -Werror)
