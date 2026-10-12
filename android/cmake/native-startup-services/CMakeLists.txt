cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsNativeStartupServices LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${BO1_ROOT}/android/cmake/native-fast-critical-section" fast-critical-section EXCLUDE_FROM_ALL)
add_library(bo1_native_startup_services STATIC
    "${BO1_ROOT}/src/universal/dvar.cpp"
    "${BO1_ROOT}/src/qcommon/cmd.cpp"
    "${BO1_ROOT}/src/qcommon/dvar_cmds.cpp"
    "${BO1_ROOT}/src/qcommon/common.cpp"
    "${BO1_ROOT}/src/client/cl_keys.cpp"
    "${BO1_ROOT}/src/universal/q_parse.cpp"
    "${BO1_ROOT}/src/universal/q_shared.cpp"
    "${BO1_ROOT}/src/universal/com_memory.cpp"
    "${BO1_ROOT}/src/universal/com_math.cpp"
    "${BO1_ROOT}/src/universal/com_shared.cpp"
    "${BO1_ROOT}/src/universal/com_hash.cpp"
    "${BO1_ROOT}/src/clientscript/cscr_stringlist.cpp"
    "${BO1_ROOT}/src/clientscript/cscr_memorytree.cpp"
    "${BO1_ROOT}/src/clientscript/cscr_instance.cpp"
    "${BO1_ROOT}/src/clientscript/cscr_debugger.cpp"
    "${BO1_ROOT}/src/platform/android/script_pool_bridge.cpp"
    "${BO1_ROOT}/src/platform/android/script_string_services.cpp"
    "${BO1_ROOT}/src/platform/android/thread_services.cpp")
if(TARGET bo1_android_hunk_services)
    target_link_libraries(bo1_native_startup_services PUBLIC bo1_android_hunk_services)
else()
    target_sources(bo1_native_startup_services PRIVATE "${BO1_ROOT}/src/platform/android/hunk_services.cpp")
endif()
target_sources(bo1_native_startup_services PRIVATE "${BO1_ROOT}/src/platform/android/legacy_crt.cpp")
target_compile_definitions(bo1_native_startup_services PRIVATE
    _DEBUG=1 BO1_STARTUP_SERVICES=1 BO1_STRING_POOL_GATE=1)
target_include_directories(bo1_native_startup_services PUBLIC "${BO1_ROOT}/src")
target_compile_features(bo1_native_startup_services PUBLIC cxx_std_20)
target_compile_options(bo1_native_startup_services PRIVATE
    -Wall -Wextra -Wpedantic -Werror=return-type -fms-extensions -fsigned-char
    -ffunction-sections -fdata-sections -fsanitize=undefined -fno-sanitize-recover=all
    -include "${BO1_ROOT}/src/platform/win32_compat/msvc_compat.h")
target_link_libraries(bo1_native_startup_services PUBLIC bo1_native_fast_critical_section)
add_executable(bo1_native_startup_services_tests "${BO1_ROOT}/tools/port/tests/native_startup_services_tests.cpp")
target_compile_definitions(bo1_native_startup_services_tests PRIVATE _DEBUG=1 BO1_STARTUP_SERVICES=1)
target_compile_options(bo1_native_startup_services_tests PRIVATE -fms-extensions -fsigned-char
    -fsanitize=undefined -fno-sanitize-recover=all -include "${BO1_ROOT}/src/platform/win32_compat/msvc_compat.h")
target_link_options(bo1_native_startup_services_tests PRIVATE -Wl,--gc-sections -fsanitize=undefined)
target_link_libraries(bo1_native_startup_services_tests PRIVATE bo1_native_startup_services)
