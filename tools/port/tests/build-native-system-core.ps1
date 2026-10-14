$ErrorActionPreference='Stop'
$taskCompiler='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$taskOut='build-android-audit/native_system_core'
$taskFlags=@('--target=aarch64-none-linux-android28',
 '--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot',
 '-std=c++20','-fms-extensions','-fsigned-char','-ffunction-sections','-fdata-sections',
 '-fsanitize=undefined','-fno-sanitize-recover=all','-D_DEBUG=1','-DBO1_NATIVE_HEADLESS_POLICY=1',
 '-DBO1_NATIVE_SYSTEM_CORE=1','-Isrc','-I.',
 '-I../DXVK-Native/include/native/directx','-I../DXVK-Native/include/native/windows',
 '-I../DXVK-Native/third_party/sdl-prefix/include/SDL2',
 '-include','src/platform/win32_compat/msvc_compat.h','-Wl,--gc-sections','-static-libstdc++',
 '-Wl,--error-limit=0','-Wl,--wrap=uname')
$taskAr='D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-ar.exe'
$taskRoot=(Get-Location).Path
$taskObjectsDirectory=Join-Path $taskRoot "$taskOut/selected_objects"
New-Item -ItemType Directory -Force $taskObjectsDirectory | Out-Null
# Reuse only actual canonical object owners from the existing production build.
# Exclude unrelated common/network/FS bodies that conflict with scoped providers.
$taskSets=@(
 @('startup-services/libbo1_native_startup_services.a','dvar.cpp.o','cmd.cpp.o','q_parse.cpp.o','com_shared.cpp.o','com_hash.cpp.o','cscr_stringlist.cpp.o','cscr_memorytree.cpp.o','cscr_instance.cpp.o','script_pool_bridge.cpp.o','script_string_services.cpp.o','thread_services.cpp.o','hunk_services.cpp.o','legacy_crt.cpp.o'),
 @('libbo1_native_entry_state.a','machine_info.cpp.o','win_configure.cpp.o','console_output.cpp.o','quit_services.cpp.o','console_input.cpp.o','startup_clock.cpp.o','win32_time.cpp.o','win32_errors.cpp.o','profiling_diagnostics.cpp.o','threads.cpp.o'),
 @('libbo1_native_database.a','dvar_cmds.cpp.o','com_memory.cpp.o','com_math.cpp.o','q_shared.cpp.o','controller_service.cpp.o','window_lifecycle.cpp.o','zone_heap.cpp.o','cl_keys.cpp.o'))
$taskObjectNames=@('win_gamepad.cpp.o')
Push-Location $taskObjectsDirectory
try {
 foreach($taskSet in $taskSets){
  $taskArchive=Join-Path $taskRoot ('build-android-audit/native_engine_entry1/'+$taskSet[0])
  $taskNames=$taskSet[1..($taskSet.Count-1)]
  & $taskAr x $taskArchive @taskNames
  if($LASTEXITCODE){throw "Cannot extract actual fixture dependency owners"}
  $taskObjectNames+=$taskNames
 }
} finally { Pop-Location }
# This historical fixture selector retains the actual canonical FCS algorithms;
# it is never applied to the full production target.
& $taskCompiler @taskFlags '-DBO1_FAST_CRIT_SECT_GATE=1' -c src/database/db_registry.cpp -o "$taskOut/fcs_fixture.o" *> "$taskOut/fcs-build.log"
if($LASTEXITCODE){Get-Content "$taskOut/fcs-build.log";exit $LASTEXITCODE}
& $taskCompiler @taskFlags -c src/win32/win_gamepad.cpp -o "$taskObjectsDirectory/win_gamepad.cpp.o" *> "$taskOut/win_gamepad-build1.log"
if($LASTEXITCODE){Get-Content "$taskOut/win_gamepad-build1.log";exit $LASTEXITCODE}
Push-Location $taskObjectsDirectory
try { & $taskAr rcs ../libcore_fixture_owners.a @taskObjectNames } finally { Pop-Location }
if($LASTEXITCODE){exit $LASTEXITCODE}
Get-FileHash ($taskObjectNames | ForEach-Object {Join-Path $taskObjectsDirectory $_}) | ConvertTo-Json | Set-Content "$taskOut/selected-objects-sha256.json"
& $taskCompiler @taskFlags tools/port/tests/native_system_core_tests.cpp src/platform/android/system_initialization.cpp src/platform/android/input_initialization.cpp src/platform/platform.cpp build-android-audit/native_system_core/fcs_fixture.o '-Wl,--start-group' build-android-audit/native_system_core/libcore_fixture_owners.a build-android-audit/native_engine_entry1/startup-services/fast-critical-section/libbo1_native_fast_critical_section.a '-Wl,--end-group' ../DXVK-Native/android/app/src/main/jniLibs/arm64-v8a/libSDL2.so '-o' "$taskOut/native_system_core_tests" *> "$taskOut/fixture-build3.log"
if($LASTEXITCODE){Select-String 'error:|undefined symbol' "$taskOut/fixture-build3.log";exit $LASTEXITCODE}
