cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsNativeCriticalSections LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_executable(bo1_native_critical_sections_tests
    "${BO1_ROOT}/src/platform/android/script_pool_bridge.cpp"
    "${BO1_ROOT}/src/platform/android/script_string_services.cpp"
    "${BO1_ROOT}/tools/port/tests/native_critical_sections_tests.cpp")
target_include_directories(bo1_native_critical_sections_tests PRIVATE "${BO1_ROOT}/src")
target_compile_features(bo1_native_critical_sections_tests PRIVATE cxx_std_20)
target_compile_options(bo1_native_critical_sections_tests PRIVATE
    -Wall -Wextra -Wpedantic -Werror=return-type -fms-extensions -fsigned-char
    -fsanitize=undefined -fno-sanitize-recover=all
    -include "${BO1_ROOT}/src/platform/win32_compat/msvc_compat.h")
target_link_options(bo1_native_critical_sections_tests PRIVATE -fsanitize=undefined)
