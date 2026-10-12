cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsNativeFastCriticalSection LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_library(bo1_native_fast_critical_section STATIC
    "${BO1_ROOT}/src/database/db_registry.cpp"
    "${BO1_ROOT}/src/universal/dvar.cpp"
    "${BO1_ROOT}/src/platform/android/fast_critical_section_services.cpp")
target_compile_definitions(bo1_native_fast_critical_section PRIVATE BO1_FAST_CRIT_SECT_GATE=1)
target_include_directories(bo1_native_fast_critical_section PUBLIC "${BO1_ROOT}/src")
target_compile_features(bo1_native_fast_critical_section PUBLIC cxx_std_20)
target_compile_options(bo1_native_fast_critical_section PRIVATE
    -Wall -Wextra -Wpedantic -Werror=return-type -fms-extensions -fsigned-char
    -fsanitize=undefined -fno-sanitize-recover=all
    -include "${BO1_ROOT}/src/platform/win32_compat/msvc_compat.h")
add_executable(bo1_native_fast_critical_section_tests
    "${BO1_ROOT}/tools/port/tests/native_fast_critical_section_tests.cpp")
target_compile_options(bo1_native_fast_critical_section_tests PRIVATE
    -Wall -Wextra -Wpedantic -Werror=return-type -fms-extensions -fsigned-char
    -fsanitize=undefined -fno-sanitize-recover=all
    -include "${BO1_ROOT}/src/platform/win32_compat/msvc_compat.h")
target_link_options(bo1_native_fast_critical_section_tests PRIVATE -fsanitize=undefined)
target_link_libraries(bo1_native_fast_critical_section_tests PRIVATE bo1_native_fast_critical_section)
