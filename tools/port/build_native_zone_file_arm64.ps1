param([Parameter(Mandatory=$true)][string]$BuildDirectory,
    [ValidateSet('O0','O3')][string]$Optimization='O0', [string]$SdkRoot='D:/Android/Sdk')
$ErrorActionPreference='Stop'
$nativeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$nativeBuild=[IO.Path]::GetFullPath((Join-Path $nativeRoot $BuildDirectory))
if (!$nativeBuild.StartsWith($nativeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Build only in Android worktree' }
if (Test-Path -LiteralPath $nativeBuild) { throw 'Use fresh output directory' }
New-Item -ItemType Directory -Path $nativeBuild | Out-Null
$nativeHashes=@{}
foreach($unit in @('src/platform/native_zone_file.cpp','src/platform/native_zone_file.h','src/platform/fastfile_inflate.cpp','src/platform/fastfile_inflate.h','src/platform/fastfile_zone.cpp','src/platform/fastfile_zone.h','src/database/db_memory.cpp','src/database/db_stream.cpp','src/platform/android/zone_memory.cpp','tools/port/tests/native_zone_file_tests.cpp','tools/port/tests/checked_zone_support.cpp','tools/port/tests/stream_init_support.cpp','tools/port/tests/hunk_test_support.cpp','tools/port/native-zone-build/CMakeLists.txt','tools/port/build_native_zone_file_arm64.ps1','android/cmake/arm64-audit/CMakeLists.txt')){
    $nativeHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $nativeRoot $unit) -Algorithm SHA256).Hash
}
$nativeHashes|ConvertTo-Json -Depth 3|Out-File (Join-Path $nativeBuild 'source-manifest.json') -Encoding utf8
$nativeCmake=Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$nativeConfigure=@('-S',(Join-Path $nativeRoot 'tools/port/native-zone-build'),'-B',$nativeBuild,'-G','Ninja',
    "-DCMAKE_MAKE_PROGRAM=$SdkRoot/cmake/3.22.1/bin/ninja.exe",
    "-DCMAKE_TOOLCHAIN_FILE=$SdkRoot/ndk/30.0.16248370/build/cmake/android.toolchain.cmake",
    '-DPython3_EXECUTABLE=C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe',
    '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-28','-DCMAKE_BUILD_TYPE=Debug',
    "-DCMAKE_C_FLAGS_DEBUG=-$Optimization -g","-DCMAKE_CXX_FLAGS_DEBUG=-$Optimization -g")
$nativeBuildArgs=@('--build',$nativeBuild,'--target','bo1_native_zone_file_tests','--verbose')
@{configure=$nativeConfigure;build=$nativeBuildArgs;optimization=$Optimization;assertions=$true;ubsan=$true}|ConvertTo-Json -Depth 5|Out-File (Join-Path $nativeBuild 'commands.json') -Encoding utf8
& $nativeCmake @nativeConfigure *> (Join-Path $nativeBuild 'configure.log')
if($LASTEXITCODE){throw 'Native zone configure failed; log retained'}
& $nativeCmake @nativeBuildArgs *> (Join-Path $nativeBuild 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $nativeBuild 'build.log') -Tail 20;throw 'Native zone build failed; log retained'}
foreach($unit in $nativeHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $nativeRoot $unit) -Algorithm SHA256).Hash -ne $nativeHashes[$unit]){throw 'Native reader source changed during build'}}
@{completed=$true;configure_exit=0;build_exit=0}|ConvertTo-Json|Out-File (Join-Path $nativeBuild 'build-result.json') -Encoding utf8
Write-Output "Native zone file ARM64 -$Optimization with assertions/UBSan: $nativeBuild"
