cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsARMCompilationAudit LANGUAGES CXX)

if(NOT ANDROID OR NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "The audit requires the Android NDK arm64-v8a toolchain.")
endif()

get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SRC_DIR "${BO1_ROOT}/src")
set(TL_DIR "${BO1_ROOT}/tl")
set(DW_DIR "${BO1_ROOT}/DemonWare")
set(BO1_SOURCE_INVENTORY_ONLY ON)
include("${BO1_ROOT}/cmake_files.cmake")

# Use the actual Windows inventory; do not copy sources or invent a second list
# of the entire engine. Selection is deliberately limited to three utilities.
set(BO1_AUDIT_SOURCES)
foreach(name IN ITEMS com_encode.cpp com_math_anglevectors.cpp timing.cpp)
    set(source "${SRC_DIR}/universal/${name}")
    if(NOT source IN_LIST UNIVERSAL_FILES)
        message(FATAL_ERROR "Audit source missing from existing inventory: ${source}")
    endif()
    list(APPEND BO1_AUDIT_SOURCES "${source}")
endforeach()

option(BO1_ARM64_COMPAT "Use the bounded Phase 1 MSVC/Win32 compatibility foundation" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_library(bo1_arm64_utilities OBJECT ${BO1_AUDIT_SOURCES})
target_include_directories(bo1_arm64_utilities PRIVATE "${SRC_DIR}")
target_compile_features(bo1_arm64_utilities PRIVATE cxx_std_20)
# Engine assertions stay enabled; never use the Windows x64 relaxation header.
target_compile_definitions(bo1_arm64_utilities PRIVATE _DEBUG=1)
target_compile_options(bo1_arm64_utilities PRIVATE
    -Wall -Wextra -Wpedantic -Wcast-align -Werror=return-type
    -ferror-limit=0 -ffp-contract=off)

if(BO1_ARM64_COMPAT)
    target_compile_options(bo1_arm64_utilities PRIVATE
        -fms-extensions -fsigned-char
        -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
    add_library(bo1_win32_compat STATIC
        "${SRC_DIR}/platform/win32_compat/win32_time.cpp"
        "${SRC_DIR}/platform/win32_compat/win32_errors.cpp")
    target_compile_features(bo1_win32_compat PRIVATE cxx_std_20)
    target_compile_options(bo1_win32_compat PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_link_libraries(bo1_arm64_utilities PRIVATE bo1_win32_compat)

    add_executable(bo1_win32_compat_tests
        "${BO1_ROOT}/tools/port/tests/win32_compat_tests.cpp"
        "${BO1_ROOT}/tools/port/tests/win32_error_tests.cpp")
    target_link_libraries(bo1_win32_compat_tests PRIVATE bo1_win32_compat)
    target_include_directories(bo1_win32_compat_tests PRIVATE "${SRC_DIR}")
    target_compile_features(bo1_win32_compat_tests PRIVATE cxx_std_20)
    target_compile_options(bo1_win32_compat_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

    add_library(bo1_arm64_abi_contract OBJECT "${BO1_ROOT}/tools/port/tests/arm64_abi_contract.cpp")
    target_include_directories(bo1_arm64_abi_contract PRIVATE "${SRC_DIR}")
    target_compile_features(bo1_arm64_abi_contract PRIVATE cxx_std_20)
    target_compile_options(bo1_arm64_abi_contract PRIVATE -Wall -Wextra -Wpedantic -fms-extensions -fsigned-char)

    add_library(bo1_arm64_narrow STATIC "${SRC_DIR}/platform/android/narrow32_android.cpp")
    target_include_directories(bo1_arm64_narrow PUBLIC "${SRC_DIR}")
    target_compile_features(bo1_arm64_narrow PUBLIC cxx_std_20)
    target_compile_options(bo1_arm64_narrow PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_executable(bo1_arm64_narrow_tests "${BO1_ROOT}/tools/port/tests/narrow32_tests.cpp")
    target_link_libraries(bo1_arm64_narrow_tests PRIVATE bo1_arm64_narrow)
    target_compile_options(bo1_arm64_narrow_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(BO1_SLOT_PROBE "${CMAKE_CURRENT_BINARY_DIR}/script_slot_probe.cpp")
    add_custom_command(OUTPUT "${BO1_SLOT_PROBE}"
        COMMAND "${Python3_EXECUTABLE}" "${BO1_ROOT}/tools/port/generate_script_slot_probe.py" "${BO1_SLOT_PROBE}"
        DEPENDS "${BO1_ROOT}/tools/port/generate_script_slot_probe.py"
                "${SRC_DIR}/clientscript/cscr_variable.h" "${SRC_DIR}/clientscript/cscr_debugger.h"
        VERBATIM)
    add_executable(bo1_script_slot_tests "${BO1_SLOT_PROBE}")
    target_link_libraries(bo1_script_slot_tests PRIVATE bo1_arm64_narrow)
    target_compile_options(bo1_script_slot_tests PRIVATE -Wall -Wextra -Wpedantic -fms-extensions -fsigned-char)

    set(BO1_FLAG_PROBE "${CMAKE_CURRENT_BINARY_DIR}/flag_guard_probe.cpp")
    # Generate a compile-only test without rewriting the retained audit evidence.
    execute_process(COMMAND "${Python3_EXECUTABLE}"
        "${BO1_ROOT}/tools/port/audit_native_width_flags.py" "${BO1_FLAG_PROBE}"
        RESULT_VARIABLE BO1_FLAG_RESULT)
    if(NOT BO1_FLAG_RESULT EQUAL 0)
        message(FATAL_ERROR "Could not generate the reviewed flag guard probe")
    endif()
    add_library(bo1_flag_guard_probe OBJECT "${BO1_FLAG_PROBE}")
    target_include_directories(bo1_flag_guard_probe PRIVATE "${SRC_DIR}")
    target_compile_features(bo1_flag_guard_probe PRIVATE cxx_std_20)

    option(BO1_ARM64_SCRIPT_HEADERS "Compile actual script headers and bounded profiler/VM utilities" ON)
    if(BO1_ARM64_SCRIPT_HEADERS)
        foreach(source IN ITEMS "${SRC_DIR}/universal/profile.cpp" "${SRC_DIR}/clientscript/cscr_instance.cpp")
            if(NOT source IN_LIST UNIVERSAL_FILES AND NOT source IN_LIST CLIENTSCRIPT_FILES)
                message(FATAL_ERROR "Gate source missing from existing inventory: ${source}")
            endif()
        endforeach()
        set(BO1_EVALUATOR_SOURCE "${SRC_DIR}/clientscript/cscr_evaluate.cpp")
        if(NOT BO1_EVALUATOR_SOURCE IN_LIST CLIENTSCRIPT_FILES)
            message(FATAL_ERROR "Evaluator missing from existing inventory")
        endif()
        add_library(bo1_evaluator_compile OBJECT "${BO1_EVALUATOR_SOURCE}")
        target_include_directories(bo1_evaluator_compile PRIVATE "${SRC_DIR}" "${BO1_ROOT}")
        target_compile_features(bo1_evaluator_compile PRIVATE cxx_std_20)
        target_compile_definitions(bo1_evaluator_compile PRIVATE _DEBUG=1)
        target_compile_options(bo1_evaluator_compile PRIVATE
            -Wall -Wextra -Wpedantic -Wcast-align -Werror=return-type -ferror-limit=0
            -fms-extensions -fsigned-char -ffunction-sections -fdata-sections
            -fsanitize=undefined -fno-sanitize-recover=all
            -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
        add_library(bo1_arm64_profile STATIC "${SRC_DIR}/universal/profile.cpp"
            "${SRC_DIR}/platform/android/profile_services.cpp")
        target_include_directories(bo1_arm64_profile PUBLIC "${SRC_DIR}")
        target_compile_features(bo1_arm64_profile PUBLIC cxx_std_20)
        target_compile_options(bo1_arm64_profile PRIVATE -Wall -Wextra -Wpedantic -Werror)
        add_executable(bo1_script_headers_tests
            "${BO1_ROOT}/tools/port/tests/script_headers_tests.cpp"
            "${SRC_DIR}/clientscript/cscr_instance.cpp")
        target_link_libraries(bo1_script_headers_tests PRIVATE bo1_arm64_narrow bo1_arm64_profile)
        target_compile_options(bo1_script_headers_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wcast-align -Werror=return-type -ferror-limit=0
            -fms-extensions -fsigned-char -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
        target_compile_definitions(bo1_script_headers_tests PRIVATE _DEBUG=1)
        add_executable(bo1_profile_tests "${BO1_ROOT}/tools/port/tests/profile_tests.cpp")
        target_link_libraries(bo1_profile_tests PRIVATE bo1_arm64_profile)
        target_compile_options(bo1_profile_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
        add_executable(bo1_math_axis_tests
            "${BO1_ROOT}/tools/port/tests/math_axis_tests.cpp"
            "${SRC_DIR}/universal/com_math_anglevectors.cpp")
        target_include_directories(bo1_math_axis_tests PRIVATE "${SRC_DIR}")
        target_compile_features(bo1_math_axis_tests PRIVATE cxx_std_20)
        target_compile_definitions(bo1_math_axis_tests PRIVATE _DEBUG=1)
        target_compile_options(bo1_math_axis_tests PRIVATE
            -Wall -Wextra -Wpedantic -Werror=array-bounds -fms-extensions -fsigned-char
            -ffp-contract=off -ffunction-sections -fdata-sections
            -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
        target_link_options(bo1_math_axis_tests PRIVATE -Wl,--gc-sections)
        set(BO1_RNG_PROBE "${CMAKE_CURRENT_BINARY_DIR}/rng_probe.cpp")
        add_custom_command(OUTPUT "${BO1_RNG_PROBE}"
            COMMAND "${Python3_EXECUTABLE}" "${BO1_ROOT}/tools/port/generate_rng_probe.py" "${BO1_RNG_PROBE}"
            DEPENDS "${BO1_ROOT}/tools/port/generate_rng_probe.py" "${SRC_DIR}/universal/com_math.cpp"
            VERBATIM)
        add_executable(bo1_rng_tests "${BO1_RNG_PROBE}")
        add_library(bo1_arm64_rng STATIC "${SRC_DIR}/platform/android/engine_random.cpp")
        target_include_directories(bo1_arm64_rng PUBLIC "${SRC_DIR}")
        target_compile_features(bo1_arm64_rng PUBLIC cxx_std_20)
        target_compile_options(bo1_arm64_rng PRIVATE -Wall -Wextra -Wpedantic -Werror)
        target_link_libraries(bo1_rng_tests PRIVATE bo1_arm64_rng)
        target_include_directories(bo1_rng_tests PRIVATE "${SRC_DIR}" "${BO1_ROOT}/tools/port/tests")
        target_compile_features(bo1_rng_tests PRIVATE cxx_std_20)
        target_compile_options(bo1_rng_tests PRIVATE -Wall -Wextra -Wpedantic -fms-extensions -fsigned-char
            -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
        add_executable(bo1_engine_rng_tests "${BO1_ROOT}/tools/port/tests/engine_rng_tests.cpp")
        target_link_libraries(bo1_engine_rng_tests PRIVATE bo1_arm64_rng)
        target_compile_options(bo1_engine_rng_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
        set(BO1_LOOKUP_SOURCE "${SRC_DIR}/clientscript/cscr_readwrite.cpp")
        if(NOT BO1_LOOKUP_SOURCE IN_LIST CLIENTSCRIPT_FILES)
            message(FATAL_ERROR "Script lookup source missing from existing inventory")
        endif()
        add_executable(bo1_script_lookup_tests "${BO1_ROOT}/tools/port/tests/script_lookup_tests.cpp" "${BO1_LOOKUP_SOURCE}")
        target_link_libraries(bo1_script_lookup_tests PRIVATE bo1_arm64_narrow bo1_arm64_profile)
        target_compile_features(bo1_script_lookup_tests PRIVATE cxx_std_20)
        target_compile_definitions(bo1_script_lookup_tests PRIVATE _DEBUG=1)
        target_compile_options(bo1_script_lookup_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wcast-align -Werror=return-type -ferror-limit=0
            -fms-extensions -fsigned-char -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
        foreach(source IN ITEMS "${SRC_DIR}/clientscript/cscr_parsetree.cpp" "${SRC_DIR}/universal/mem_userhunk.cpp")
            if(NOT source IN_LIST CLIENTSCRIPT_FILES AND NOT source IN_LIST UNIVERSAL_FILES)
                message(FATAL_ERROR "Parse/allocator source missing from existing inventory: ${source}")
            endif()
        endforeach()
        add_library(bo1_parsetree_compile OBJECT "${SRC_DIR}/clientscript/cscr_parsetree.cpp"
            "${SRC_DIR}/universal/mem_userhunk.cpp")
        target_include_directories(bo1_parsetree_compile PRIVATE "${SRC_DIR}")
        target_compile_features(bo1_parsetree_compile PRIVATE cxx_std_20)
        target_compile_definitions(bo1_parsetree_compile PRIVATE _DEBUG=1)
        target_compile_options(bo1_parsetree_compile PRIVATE
            -Wall -Wextra -Wpedantic -Wcast-align -Werror=return-type -ferror-limit=0
            -fms-extensions -fsigned-char -ffunction-sections -fdata-sections
            -fsanitize=undefined -fno-sanitize-recover=all
            -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
        add_library(bo1_android_virtual_memory STATIC "${SRC_DIR}/platform/android/virtual_memory.cpp")
        target_include_directories(bo1_android_virtual_memory PUBLIC "${SRC_DIR}")
        target_compile_features(bo1_android_virtual_memory PUBLIC cxx_std_20)
        target_compile_options(bo1_android_virtual_memory PRIVATE -Wall -Wextra -Wpedantic -Werror)
        add_library(bo1_android_hunk_services STATIC "${SRC_DIR}/platform/android/hunk_services.cpp")
        target_include_directories(bo1_android_hunk_services PUBLIC "${SRC_DIR}")
        target_compile_features(bo1_android_hunk_services PUBLIC cxx_std_20)
        target_compile_options(bo1_android_hunk_services PRIVATE -Wall -Wextra -Wpedantic -Werror)
        foreach(probe IN ITEMS parsetree hunk)
            add_executable(bo1_${probe}_tests
                "${BO1_ROOT}/tools/port/tests/${probe}_tests.cpp"
                "${BO1_ROOT}/tools/port/tests/hunk_test_support.cpp"
                $<TARGET_OBJECTS:bo1_parsetree_compile>)
            target_link_libraries(bo1_${probe}_tests PRIVATE bo1_arm64_narrow bo1_android_virtual_memory bo1_android_hunk_services)
            target_compile_definitions(bo1_${probe}_tests PRIVATE _DEBUG=1)
            target_compile_options(bo1_${probe}_tests PRIVATE
                -Wall -Wextra -Wpedantic -Werror=return-type -fms-extensions -fsigned-char
                -ffunction-sections -fdata-sections -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
            target_link_options(bo1_${probe}_tests PRIVATE -Wl,--gc-sections -fsanitize=undefined)
        endforeach()
        set(BO1_FIRSTFIT_SOURCE "${SRC_DIR}/universal/mem_firstfit.cpp")
        if(NOT BO1_FIRSTFIT_SOURCE IN_LIST UNIVERSAL_FILES)
            message(FATAL_ERROR "First-fit source missing from existing inventory")
        endif()
        add_library(bo1_firstfit_compile OBJECT "${BO1_FIRSTFIT_SOURCE}")
        target_include_directories(bo1_firstfit_compile PRIVATE "${SRC_DIR}")
        target_compile_features(bo1_firstfit_compile PRIVATE cxx_std_20)
        target_compile_definitions(bo1_firstfit_compile PRIVATE _DEBUG=1)
        target_compile_options(bo1_firstfit_compile PRIVATE
            -Wall -Wextra -Wpedantic -Wcast-align -Werror=return-type -fms-extensions -fsigned-char
            -fsanitize=undefined -fno-sanitize-recover=all
            -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
        add_library(bo1_android_zone_heap STATIC "${SRC_DIR}/platform/android/zone_heap.cpp")
        target_include_directories(bo1_android_zone_heap PUBLIC "${SRC_DIR}")
        target_compile_features(bo1_android_zone_heap PUBLIC cxx_std_20)
        target_compile_options(bo1_android_zone_heap PRIVATE -Wall -Wextra -Wpedantic -Werror)
        foreach(probe IN ITEMS debugger firstfit)
            add_executable(bo1_${probe}_tests "${BO1_ROOT}/tools/port/tests/${probe}_tests.cpp"
                "${BO1_ROOT}/tools/port/tests/hunk_test_support.cpp"
                $<TARGET_OBJECTS:bo1_parsetree_compile>)
            target_link_libraries(bo1_${probe}_tests PRIVATE bo1_arm64_narrow bo1_android_virtual_memory bo1_android_hunk_services)
            target_compile_definitions(bo1_${probe}_tests PRIVATE _DEBUG=1)
            target_compile_options(bo1_${probe}_tests PRIVATE
                -Wall -Wextra -Wpedantic -Werror=return-type -fms-extensions -fsigned-char
                -ffunction-sections -fdata-sections -fsanitize=undefined -fno-sanitize-recover=all
                -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
            target_link_options(bo1_${probe}_tests PRIVATE -Wl,--gc-sections -fsanitize=undefined)
        endforeach()
        target_sources(bo1_debugger_tests PRIVATE $<TARGET_OBJECTS:bo1_evaluator_compile>)
        set(BO1_EVALUATOR_TEST_SERVICES "${CMAKE_CURRENT_BINARY_DIR}/evaluator_test_services.cpp")
        add_custom_command(OUTPUT "${BO1_EVALUATOR_TEST_SERVICES}"
            COMMAND "${Python3_EXECUTABLE}" "${BO1_ROOT}/tools/port/generate_evaluator_test_services.py" "${BO1_EVALUATOR_TEST_SERVICES}"
            DEPENDS "${BO1_ROOT}/tools/port/generate_evaluator_test_services.py"
                    "${BO1_ROOT}/tools/port/tests/evaluator_unavailable_services.json"
            VERBATIM)
        target_sources(bo1_debugger_tests PRIVATE "${BO1_EVALUATOR_TEST_SERVICES}")
        target_compile_definitions(bo1_debugger_tests PRIVATE BO1_DEBUGGER_GATE=1)
        target_link_libraries(bo1_debugger_tests PRIVATE bo1_android_zone_heap)
        target_sources(bo1_firstfit_tests PRIVATE $<TARGET_OBJECTS:bo1_firstfit_compile>)
        target_compile_definitions(bo1_firstfit_tests PRIVATE BO1_REAL_FIRSTFIT_GATE=1)
    endif()
endif()

add_library(bo1_arm64_narrow_slot_probe OBJECT
    "${BO1_ROOT}/tools/port/tests/arm64_narrow_slot_probe.cpp")
target_include_directories(bo1_arm64_narrow_slot_probe PRIVATE "${SRC_DIR}")
target_compile_features(bo1_arm64_narrow_slot_probe PRIVATE cxx_std_20)
