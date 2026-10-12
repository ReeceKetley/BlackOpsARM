cmake_minimum_required(VERSION 3.22.1)
project(NativeZoneFileGate LANGUAGES C CXX)
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SRC_DIR "${BO1_ROOT}/src")
# Reuse existing production compilation selections and native allocator services.
# Test support is linked only into this executable, never into a runtime library.
add_subdirectory("${BO1_ROOT}/android/cmake/arm64-audit" audit EXCLUDE_FROM_ALL)
add_executable(bo1_native_zone_file_tests
    "${BO1_ROOT}/tools/port/tests/native_zone_file_tests.cpp"
    "${BO1_ROOT}/tools/port/tests/checked_zone_support.cpp"
    "${BO1_ROOT}/tools/port/tests/hunk_test_support.cpp"
    "${BO1_ROOT}/tools/port/tests/stream_init_support.cpp"
    "${SRC_DIR}/platform/native_zone_file.cpp"
    "${SRC_DIR}/platform/fastfile_zone.cpp"
    "${SRC_DIR}/platform/fastfile_inflate.cpp"
    "${SRC_DIR}/platform/android/zone_memory.cpp"
    "${SRC_DIR}/platform/android/database_diagnostics.cpp"
    $<TARGET_OBJECTS:bo1_zone_compile> $<TARGET_OBJECTS:bo1_parsetree_compile>
    $<TARGET_OBJECTS:bo1_firstfit_compile>)
target_include_directories(bo1_native_zone_file_tests PRIVATE "${SRC_DIR}")
target_compile_features(bo1_native_zone_file_tests PRIVATE cxx_std_20)
target_compile_definitions(bo1_native_zone_file_tests PRIVATE _DEBUG=1 BO1_ZONE_GATE=1
    BO1_REAL_FIRSTFIT_GATE=1 BO1_DEBUGGER_GATE=1 BO1_CHECKED_ZONE_GATE=1)
target_compile_options(bo1_native_zone_file_tests PRIVATE -Wall -Wextra -Wpedantic -Werror=return-type
    -fms-extensions -fsigned-char -ffunction-sections -fdata-sections
    -fsanitize=undefined -fno-sanitize-recover=all
    -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
target_link_libraries(bo1_native_zone_file_tests PRIVATE bo1_arm64_narrow bo1_android_virtual_memory
    bo1_android_hunk_services bo1_fastfile_zlib)
target_link_options(bo1_native_zone_file_tests PRIVATE -Wl,--gc-sections -fsanitize=undefined)
