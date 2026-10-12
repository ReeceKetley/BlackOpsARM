cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(BlackOpsNativeStartupLoader LANGUAGES C CXX)
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SRC_DIR "${BO1_ROOT}/src")
option(BO1_NATIVE_THREAD_INTEGRATION "Validate actual native main TLS and MP71 in one process" OFF)
add_subdirectory("${BO1_ROOT}/android/cmake/arm64-audit" audit EXCLUDE_FROM_ALL)

# Reusable production component. It contains no tools/port source, fixture Dvar,
# test failure injection or manufactured engine initialization. Its unresolved
# engine services are supplied by the genuine application or the explicit driver.
add_library(bo1_native_startup_loader STATIC
    "${SRC_DIR}/platform/native_zone_load.cpp"
    "${SRC_DIR}/platform/native_zone_file.cpp"
    "${SRC_DIR}/platform/fastfile_assetlist.cpp"
    "${SRC_DIR}/platform/fastfile_asset_directory.cpp"
    "${SRC_DIR}/platform/fastfile_stringtable_header.cpp"
    "${SRC_DIR}/platform/fastfile_stringtable_cells.cpp"
    "${SRC_DIR}/platform/fastfile_stringtable_indices.cpp"
    "${SRC_DIR}/platform/fastfile_raw_string.cpp"
    "${SRC_DIR}/platform/fastfile_initialized_strings.cpp"
    "${SRC_DIR}/platform/fastfile_rawfile_header.cpp"
    "${SRC_DIR}/platform/fastfile_rawfile_payload.cpp"
    "${SRC_DIR}/platform/fastfile_zone.cpp"
    "${SRC_DIR}/platform/fastfile_inflate.cpp"
    "${SRC_DIR}/platform/android/zone_memory.cpp"
    "${SRC_DIR}/platform/android/database_diagnostics.cpp"
    "${SRC_DIR}/platform/android/script_pool_bridge.cpp"
    "${SRC_DIR}/platform/android/script_string_services.cpp"
    "${SRC_DIR}/database/db_file_load.cpp" "${SRC_DIR}/database/db_load.cpp"
    "${SRC_DIR}/database/db_registry.cpp" "${SRC_DIR}/database/db_assetnames.cpp"
    "${SRC_DIR}/database/db_expand.cpp" "${SRC_DIR}/database/db_stream_load.cpp"
    "${SRC_DIR}/universal/com_stringtable.cpp" "${SRC_DIR}/universal/com_stringtable_obj.cpp"
    "${SRC_DIR}/universal/com_stringtable_sort.cpp" "${SRC_DIR}/universal/com_hash.cpp"
    "${SRC_DIR}/universal/q_shared.cpp" "${SRC_DIR}/platform/win32_compat/sort16.cpp"
    $<TARGET_OBJECTS:bo1_zone_compile> $<TARGET_OBJECTS:bo1_parsetree_compile>
    $<TARGET_OBJECTS:bo1_firstfit_compile>)
target_include_directories(bo1_native_startup_loader PUBLIC "${SRC_DIR}")
target_compile_features(bo1_native_startup_loader PUBLIC cxx_std_20)
target_compile_definitions(bo1_native_startup_loader PRIVATE _DEBUG=1
    BO1_NATIVE_ZONE_LOAD=1 BO1_ASSETLIST_GATE=1 BO1_FRONTEND_TABLES_GATE=1
    BO1_RAW_STRING_GATE=1 BO1_ASSET_DIRECTORY_GATE=1 BO1_STRINGTABLE_HEADER_GATE=1
    BO1_STRINGTABLE_CELLS_GATE=1 BO1_STRINGTABLE_INDEX_GATE=1 BO1_STRINGTABLE_HASH_GATE=1
    BO1_STRINGTABLE_LOOKUP_GATE=1 BO1_STRINGTABLE_NORMALIZATION_GATE=1
    BO1_STRINGTABLE_OWNER_GATE=1 BO1_RAWFILE_HEADER_GATE=1 BO1_RAWFILE_PAYLOAD_GATE=1
    BO1_RAWFILE_OWNER_GATE=1 BO1_ASSET_ENTRY_GATE=1 BO1_ENTRY_ALLOC_GATE=1
    BO1_REGISTRY_LOOKUP_GATE=1 BO1_REGISTRY_INSERT_GATE=1 BO1_REGISTRY_ADD_GATE=1)
target_compile_options(bo1_native_startup_loader PRIVATE -Wall -Wextra -Wpedantic
    -Werror=return-type -fms-extensions -fsigned-char -ffunction-sections -fdata-sections
    -fsanitize=undefined -fno-sanitize-recover=all
    -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
target_link_libraries(bo1_native_startup_loader PUBLIC bo1_arm64_narrow
    bo1_android_virtual_memory bo1_android_hunk_services bo1_fastfile_zlib)

# Controlled caller/error inputs belong solely to this validation executable.
add_executable(bo1_native_startup_zone_tests
    "${BO1_ROOT}/tools/port/tests/native_startup_zone_tests.cpp"
    "${BO1_ROOT}/tools/port/tests/entry_owner_support.cpp"
    "${BO1_ROOT}/tools/port/tests/checked_zone_support.cpp"
    "${BO1_ROOT}/tools/port/tests/hunk_test_support.cpp")
target_compile_definitions(bo1_native_startup_zone_tests PRIVATE _DEBUG=1
    BO1_NATIVE_ZONE_LOAD=1 BO1_ASSETLIST_GATE=1 BO1_STRINGTABLE_INDEX_GATE=1
    BO1_STRINGTABLE_OWNER_GATE=1 BO1_RAWFILE_PAYLOAD_GATE=1 BO1_ASSET_ENTRY_GATE=1
    BO1_ENTRY_ALLOC_GATE=1 BO1_REGISTRY_ADD_GATE=1 BO1_REGISTRY_LOOKUP_GATE=1
    BO1_STRINGTABLE_LOOKUP_GATE=1 BO1_CHECKED_ZONE_GATE=1
    BO1_ZONE_GATE=1 BO1_REAL_FIRSTFIT_GATE=1 BO1_DEBUGGER_GATE=1)
target_compile_options(bo1_native_startup_zone_tests PRIVATE -fms-extensions
    -fsigned-char -fsanitize=undefined -fno-sanitize-recover=all
    -include "${SRC_DIR}/platform/win32_compat/msvc_compat.h")
target_link_libraries(bo1_native_startup_zone_tests PRIVATE bo1_native_startup_loader)

if(BO1_NATIVE_THREAD_INTEGRATION)
    add_subdirectory("${BO1_ROOT}/android/cmake/native-thread-data" native-thread-data EXCLUDE_FROM_ALL)
    target_compile_definitions(bo1_native_startup_zone_tests PRIVATE BO1_NATIVE_THREAD_INTEGRATION=1)
    target_link_libraries(bo1_native_startup_zone_tests PRIVATE bo1_native_thread_data)
endif()
target_link_options(bo1_native_startup_zone_tests PRIVATE -Wl,--gc-sections -fsanitize=undefined)
