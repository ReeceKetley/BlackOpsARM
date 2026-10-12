cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsNativeThreadData LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

# Reusable production storage, original initializer/reset and native bindings.
# No test implementation is compiled into this library.
add_library(bo1_native_thread_data STATIC
    "${BO1_ROOT}/src/universal/q_shared.cpp"
    "${BO1_ROOT}/src/qcommon/cmd.cpp"
    "${BO1_ROOT}/src/platform/android/thread_services.cpp")
if(TARGET bo1_android_hunk_services)
    target_link_libraries(bo1_native_thread_data PUBLIC bo1_android_hunk_services)
else()
    target_sources(bo1_native_thread_data PRIVATE "${BO1_ROOT}/src/platform/android/hunk_services.cpp")
endif()
target_compile_definitions(bo1_native_thread_data PRIVATE BO1_NATIVE_THREAD_DATA=1)
target_include_directories(bo1_native_thread_data PUBLIC "${BO1_ROOT}/src")
target_compile_features(bo1_native_thread_data PUBLIC cxx_std_20)
target_compile_options(bo1_native_thread_data PRIVATE
    -Wall -Wextra -Wpedantic -Werror=return-type -fms-extensions -fsigned-char
    -fsanitize=undefined -fno-sanitize-recover=all
    -include "${BO1_ROOT}/src/platform/win32_compat/msvc_compat.h")

add_executable(bo1_native_thread_data_tests
    "${BO1_ROOT}/tools/port/tests/native_thread_data_tests.cpp")
target_compile_options(bo1_native_thread_data_tests PRIVATE
    -Wall -Wextra -Wpedantic -Werror=return-type -fms-extensions -fsigned-char
    -fsanitize=undefined -fno-sanitize-recover=all
    -include "${BO1_ROOT}/src/platform/win32_compat/msvc_compat.h")
target_link_options(bo1_native_thread_data_tests PRIVATE -fsanitize=undefined)
target_link_libraries(bo1_native_thread_data_tests PRIVATE bo1_native_thread_data)
