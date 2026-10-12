param([Parameter(Mandatory=$true)][string]$BuildDirectory,
    [ValidateSet('O0','O3')][string]$Optimization='O0', [string]$SdkRoot='D:/Android/Sdk',
    [switch]$ThreadIntegration)
$ErrorActionPreference='Stop'
$nativeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$nativeBuild=[IO.Path]::GetFullPath((Join-Path $nativeRoot $BuildDirectory))
if (!$nativeBuild.StartsWith($nativeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Build only in Android worktree' }
if (Test-Path -LiteralPath $nativeBuild) { throw 'Use fresh output directory' }
New-Item -ItemType Directory -Path $nativeBuild | Out-Null
$nativeHashes=@{}
foreach($unit in @('src/platform/native_zone_file.cpp','src/platform/native_zone_file.h','src/platform/native_zone_load.cpp','src/platform/native_zone_load.h','src/platform/fastfile_inflate.cpp','src/platform/fastfile_inflate.h','src/platform/fastfile_zone.cpp','src/platform/fastfile_zone.h','src/database/db_memory.cpp','src/database/db_stream.cpp','src/database/db_stream_load.cpp','src/database/db_file_load.cpp','src/database/db_load.cpp','src/database/db_expand.cpp','src/database/db_registry.cpp','src/database/db_assetnames.cpp','src/database/db_native_zone_load.h','src/platform/android/zone_memory.cpp','tools/port/tests/native_startup_zone_tests.cpp','tools/port/tests/entry_owner_support.cpp','tools/port/tests/checked_zone_support.cpp','tools/port/tests/hunk_test_support.cpp','android/cmake/native-startup-loader/CMakeLists.txt','tools/port/build_native_startup_zone_arm64.ps1','android/cmake/arm64-audit/CMakeLists.txt')){
    $nativeHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $nativeRoot $unit) -Algorithm SHA256).Hash
}
$nativeHashes|ConvertTo-Json -Depth 3|Out-File (Join-Path $nativeBuild 'source-manifest.json') -Encoding utf8
if($ThreadIntegration){
    foreach($unit in @('src/qcommon/cmd.cpp','src/qcommon/cmd_args.h','src/qcommon/trace_thread_info.h','src/qcommon/thread_context.h','src/universal/thread_data.h','src/platform/android/thread_services.cpp','src/platform/android/thread_services.h','src/platform/android/hunk_services.cpp','src/platform/android/hunk_services.h','android/cmake/native-thread-data/CMakeLists.txt')){
        $nativeHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $nativeRoot $unit) -Algorithm SHA256).Hash
    }
}
$nativeCmake=Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$nativeConfigure=@('-S',(Join-Path $nativeRoot 'android/cmake/native-startup-loader'),'-B',$nativeBuild,'-G','Ninja',
    "-DCMAKE_MAKE_PROGRAM=$SdkRoot/cmake/3.22.1/bin/ninja.exe",
    "-DCMAKE_TOOLCHAIN_FILE=$SdkRoot/ndk/30.0.16248370/build/cmake/android.toolchain.cmake",
    '-DPython3_EXECUTABLE=C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe',
    '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-28','-DCMAKE_BUILD_TYPE=Debug',
    "-DCMAKE_C_FLAGS_DEBUG=-$Optimization -g","-DCMAKE_CXX_FLAGS_DEBUG=-$Optimization -g",
    $(if($ThreadIntegration){'-DBO1_NATIVE_THREAD_INTEGRATION=ON'}else{'-DBO1_NATIVE_THREAD_INTEGRATION=OFF'}))
$nativeBuildArgs=@('--build',$nativeBuild,'--target','bo1_native_startup_zone_tests','--verbose')
@{configure=$nativeConfigure;build=$nativeBuildArgs;optimization=$Optimization;assertions=$true;ubsan=$true;thread_integration=[bool]$ThreadIntegration}|ConvertTo-Json -Depth 5|Out-File (Join-Path $nativeBuild 'commands.json') -Encoding utf8
& $nativeCmake @nativeConfigure *> (Join-Path $nativeBuild 'configure.log')
if($LASTEXITCODE){throw 'Native zone configure failed; log retained'}
$nativeCompileCommands=Get-Content -LiteralPath (Join-Path $nativeBuild 'compile_commands.json') -Raw|ConvertFrom-Json
foreach($entry in $nativeCompileCommands){
    if($entry.command -match 'CMakeFiles[\\/](bo1_native_startup_loader|bo1_native_startup_zone_tests|bo1_native_thread_data|bo1_zone_compile|bo1_parsetree_compile|bo1_firstfit_compile|bo1_android_hunk_services|bo1_android_virtual_memory|bo1_fastfile_zlib|bo1_arm64_narrow)\.dir[\\/]' -and (Test-Path -LiteralPath $entry.file)){
        $nativeHashes[$entry.file]=(Get-FileHash -LiteralPath $entry.file -Algorithm SHA256).Hash
    }
}
$nativeHashes|ConvertTo-Json -Depth 3|Out-File (Join-Path $nativeBuild 'source-manifest.json') -Encoding utf8
& $nativeCmake @nativeBuildArgs *> (Join-Path $nativeBuild 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $nativeBuild 'build.log') -Tail 20;throw 'Native zone build failed; log retained'}
foreach($unit in $nativeHashes.Keys){$nativeInput=if([IO.Path]::IsPathRooted($unit)){$unit}else{Join-Path $nativeRoot $unit};if((Get-FileHash -LiteralPath $nativeInput -Algorithm SHA256).Hash -ne $nativeHashes[$unit]){throw 'Native startup source changed during build'}}
@{completed=$true;configure_exit=0;build_exit=0}|ConvertTo-Json|Out-File (Join-Path $nativeBuild 'build-result.json') -Encoding utf8
Write-Output "Native startup loader ARM64 -$Optimization with assertions/UBSan: $nativeBuild"
