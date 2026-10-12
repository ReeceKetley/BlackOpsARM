cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsNativeStartupServicesZone LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
# The loader creates the canonical hunk service target before services select
# their dependency, giving the entire process exactly one owner implementation.
add_subdirectory("${BO1_ROOT}/android/cmake/native-startup-loader" loader EXCLUDE_FROM_ALL)
add_subdirectory("${BO1_ROOT}/android/cmake/native-startup-services" services EXCLUDE_FROM_ALL)
add_executable(bo1_native_startup_services_zone_tests
    "${BO1_ROOT}/tools/port/tests/native_startup_services_zone_tests.cpp"
    "${BO1_ROOT}/tools/port/tests/native_startup_zone_tests.cpp"
    "${BO1_ROOT}/tools/port/tests/entry_owner_support.cpp"
    "${BO1_ROOT}/tools/port/tests/checked_zone_support.cpp"
    "${BO1_ROOT}/tools/port/tests/hunk_test_support.cpp")
target_compile_definitions(bo1_native_startup_services_zone_tests PRIVATE _DEBUG=1
    BO1_STARTUP_SERVICES=1 BO1_STARTUP_SERVICES_ZONE_INTEGRATION=1
    BO1_NATIVE_ZONE_LOAD=1 BO1_ASSETLIST_GATE=1 BO1_STRINGTABLE_INDEX_GATE=1
    BO1_STRINGTABLE_OWNER_GATE=1 BO1_RAWFILE_PAYLOAD_GATE=1 BO1_ASSET_ENTRY_GATE=1
    BO1_ENTRY_ALLOC_GATE=1 BO1_REGISTRY_ADD_GATE=1 BO1_REGISTRY_LOOKUP_GATE=1
    BO1_STRINGTABLE_LOOKUP_GATE=1 BO1_CHECKED_ZONE_GATE=1 BO1_FRONTEND_INTERN_GATE=1
    BO1_ZONE_GATE=1 BO1_REAL_FIRSTFIT_GATE=1 BO1_DEBUGGER_GATE=1)
target_compile_options(bo1_native_startup_services_zone_tests PRIVATE -fms-extensions
    -fsigned-char -fsanitize=undefined -fno-sanitize-recover=all
    -include "${BO1_ROOT}/src/platform/win32_compat/msvc_compat.h")
target_link_options(bo1_native_startup_services_zone_tests PRIVATE -Wl,--gc-sections -fsanitize=undefined -Wl,-Map,startup-services-zone.map)
target_link_libraries(bo1_native_startup_services_zone_tests PRIVATE
    bo1_native_startup_services bo1_native_startup_loader)
