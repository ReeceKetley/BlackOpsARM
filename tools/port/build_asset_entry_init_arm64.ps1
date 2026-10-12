param([string]$SdkRoot='D:/Android/Sdk', [string]$PythonExe='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe', [ValidateSet('O0','O3')][string]$Optimization='O3', [Parameter(Mandatory=$true)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$auditRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$auditBuild=[IO.Path]::GetFullPath((Join-Path $auditRoot $BuildDirectory))
if (!$auditBuild.StartsWith($auditRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Only Android worktree outputs allowed' }
if(Test-Path -LiteralPath $auditBuild){throw 'Use a fresh output directory'}
New-Item -ItemType Directory -Path $auditBuild | Out-Null
$inputHashes=@{}
foreach($unit in @('tools/port/tests/asset_entry_init_tests.cpp','src/database/db_registry.cpp','src/platform/android/script_pool_bridge.cpp','src/platform/android/script_string_services.cpp','src/database/db_asset_entry_types.h','src/database/db_asset_pool.h','src/database/db_assetlist_types.h','src/database/database.h','src/database/db_rawfile_types.h','src/universal/com_stringtable.h','src/universal/assertive.h','src/universal/critical_sections.h','src/platform/critical_section_services.h','src/platform/android/script_string_services.h','src/platform/bo1_native_width.h','src/platform/win32_compat/msvc_compat.h','android/cmake/arm64-audit/CMakeLists.txt')){$inputHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $auditRoot $unit) -Algorithm SHA256).Hash}
$inputHashes[$PSCommandPath]=(Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
$inputHashes|ConvertTo-Json -Depth 4|Out-File (Join-Path $auditBuild 'source-manifest.json') -Encoding utf8
$auditCmake=Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$auditNinja=Join-Path $SdkRoot 'cmake/3.22.1/bin/ninja.exe'
$auditToolchain=Join-Path $SdkRoot 'ndk/30.0.16248370/build/cmake/android.toolchain.cmake'
$configureArgs=@('-S',(Join-Path $auditRoot 'android/cmake/arm64-audit'),'-B',$auditBuild,'-G','Ninja',
    "-DCMAKE_MAKE_PROGRAM=$auditNinja","-DCMAKE_TOOLCHAIN_FILE=$auditToolchain",
    "-DPython3_EXECUTABLE=$PythonExe",'-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-28',
    '-DCMAKE_BUILD_TYPE=Debug','-DBO1_ARM64_COMPAT=ON',"-DCMAKE_C_FLAGS_DEBUG=-$Optimization -g","-DCMAKE_CXX_FLAGS_DEBUG=-$Optimization -g")
$buildArgs=@('--build',$auditBuild,'--target','bo1_asset_entry_init_tests','--verbose')
@{optimization=$Optimization;assertions=$true;ubsan=$true;configure=@{executable=$auditCmake;argv=$configureArgs};build=@{executable=$auditCmake;argv=$buildArgs}} |
    ConvertTo-Json -Depth 5 | Out-File (Join-Path $auditBuild 'commands.json') -Encoding utf8
& $auditCmake @configureArgs `
    2>&1 | Out-File (Join-Path $auditBuild 'configure.log') -Encoding utf8
$configureExit=$LASTEXITCODE
@{configure_exit=$configureExit;build_exit=$null;completed=$false} | ConvertTo-Json |
    Out-File (Join-Path $auditBuild 'build-result.json') -Encoding utf8
if($configureExit){throw 'Optimized configure failed; log retained'}
& $auditCmake @buildArgs `
    2>&1 | Out-File (Join-Path $auditBuild 'build.log') -Encoding utf8
$buildExit=$LASTEXITCODE
foreach($unit in $inputHashes.Keys){$absolute=if([IO.Path]::IsPathRooted($unit)){$unit}else{Join-Path $auditRoot $unit};if((Get-FileHash -LiteralPath $absolute -Algorithm SHA256).Hash -ne $inputHashes[$unit]){throw 'Source changed during build; logs preserved'}}
@{configure_exit=$configureExit;build_exit=$buildExit;completed=($buildExit -eq 0)} | ConvertTo-Json |
    Out-File (Join-Path $auditBuild 'build-result.json') -Encoding utf8
if($buildExit){Get-Content (Join-Path $auditBuild 'build.log') -Tail 20;throw 'Optimized build failed; log retained'}
Write-Output "ARM64 -$Optimization with assertions/UBSan: $auditBuild"
