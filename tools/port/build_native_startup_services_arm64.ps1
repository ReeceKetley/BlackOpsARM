param([Parameter(Mandatory=$true)][string]$BuildDirectory,
    [ValidateSet('O0','O3')][string]$Optimization='O0', [string]$SdkRoot='D:/Android/Sdk')
$ErrorActionPreference='Stop'
$nativeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$nativeBuild=[IO.Path]::GetFullPath((Join-Path $nativeRoot $BuildDirectory))
if (!$nativeBuild.StartsWith($nativeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Build only in Android worktree' }
if (Test-Path -LiteralPath $nativeBuild) { throw 'Use fresh output directory' }
New-Item -ItemType Directory -Path $nativeBuild | Out-Null
$nativeHashes=@{}
foreach($unit in @('src/universal/dvar.cpp','src/qcommon/cmd.cpp','src/qcommon/dvar_cmds.cpp','src/qcommon/common.cpp','src/client/cl_keys.cpp','src/universal/q_parse.cpp','src/universal/q_shared.cpp','src/universal/com_memory.cpp','src/universal/com_math.cpp','src/universal/com_shared.cpp','src/universal/com_hash.cpp','src/clientscript/cscr_stringlist.cpp','src/clientscript/cscr_memorytree.cpp','src/clientscript/cscr_instance.cpp','src/clientscript/cscr_debugger.cpp','src/platform/android/script_pool_bridge.cpp','src/platform/android/script_string_services.cpp','src/platform/android/thread_services.cpp','src/platform/android/legacy_crt.cpp','src/platform/android/fast_critical_section_services.cpp','src/platform/startup_services_api.h','src/platform/android/legacy_crt.h','src/qcommon/cmd.h','src/universal/thread_data.h','src/qcommon/cmd_args.h','src/qcommon/thread_context.h','src/qcommon/trace_thread_info.h','src/universal/dvar.h','src/universal/q_parse.h','src/qcommon/dvar_cmds.h','src/clientscript/cscr_debugger.h','src/platform/fast_critical_section.h','android/cmake/native-startup-services/CMakeLists.txt','src/database/db_registry.cpp','src/platform/android/hunk_services.cpp','src/platform/android/hunk_services.h','tools/port/tests/native_startup_services_tests.cpp','tools/port/build_native_startup_services_arm64.ps1','android/cmake/native-fast-critical-section/CMakeLists.txt')){
    $nativeHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $nativeRoot $unit) -Algorithm SHA256).Hash
}
$nativeHashes|ConvertTo-Json -Depth 3|Out-File (Join-Path $nativeBuild 'source-manifest.json') -Encoding utf8
$nativeCmake=Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$nativeConfigure=@('-S',(Join-Path $nativeRoot 'android/cmake/native-startup-services'),'-B',$nativeBuild,'-G','Ninja',
    "-DCMAKE_MAKE_PROGRAM=$SdkRoot/cmake/3.22.1/bin/ninja.exe",
    "-DCMAKE_TOOLCHAIN_FILE=$SdkRoot/ndk/30.0.16248370/build/cmake/android.toolchain.cmake",
    '-DPython3_EXECUTABLE=C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe',
    '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-28','-DCMAKE_BUILD_TYPE=Debug',
    "-DCMAKE_C_FLAGS_DEBUG=-$Optimization -g","-DCMAKE_CXX_FLAGS_DEBUG=-$Optimization -g")
$nativeBuildArgs=@('--build',$nativeBuild,'--target','bo1_native_startup_services_tests','--verbose')
@{configure=$nativeConfigure;build=$nativeBuildArgs;optimization=$Optimization;assertions=$true;ubsan=$true}|ConvertTo-Json -Depth 5|Out-File (Join-Path $nativeBuild 'commands.json') -Encoding utf8
& $nativeCmake @nativeConfigure *> (Join-Path $nativeBuild 'configure.log')
if($LASTEXITCODE){throw 'Native startup services configure failed; log retained'}
& $nativeCmake @nativeBuildArgs *> (Join-Path $nativeBuild 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $nativeBuild 'build.log') -Tail 20;throw 'Native startup services build failed; log retained'}
foreach($unit in $nativeHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $nativeRoot $unit) -Algorithm SHA256).Hash -ne $nativeHashes[$unit]){throw 'Native startup services source changed during build'}}
@{completed=$true;configure_exit=0;build_exit=0}|ConvertTo-Json|Out-File (Join-Path $nativeBuild 'build-result.json') -Encoding utf8
Write-Output "Native startup services file ARM64 -$Optimization with assertions/UBSan: $nativeBuild"
