param([string]$SdkRoot='D:/Android/Sdk', [string]$PythonExe='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe', [Parameter(Mandatory=$true)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$auditRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$auditBuild=[IO.Path]::GetFullPath((Join-Path $auditRoot $BuildDirectory))
if (!$auditBuild.StartsWith($auditRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Only Android worktree outputs allowed' }
if(Test-Path -LiteralPath $auditBuild){throw 'Use a fresh output directory'}
New-Item -ItemType Directory -Path $auditBuild | Out-Null
$auditCmake=Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$auditNinja=Join-Path $SdkRoot 'cmake/3.22.1/bin/ninja.exe'
$auditToolchain=Join-Path $SdkRoot 'ndk/30.0.16248370/build/cmake/android.toolchain.cmake'
& $auditCmake -S (Join-Path $auditRoot 'android/cmake/arm64-audit') -B $auditBuild -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$auditNinja" "-DCMAKE_TOOLCHAIN_FILE=$auditToolchain" `
    "-DPython3_EXECUTABLE=$PythonExe" `
    '-DANDROID_ABI=arm64-v8a' '-DANDROID_PLATFORM=android-28' '-DCMAKE_BUILD_TYPE=Debug' `
    '-DBO1_ARM64_COMPAT=ON' '-DCMAKE_C_FLAGS_DEBUG=-O3 -g' '-DCMAKE_CXX_FLAGS_DEBUG=-O3 -g' `
    2>&1 | Out-File (Join-Path $auditBuild 'configure.log') -Encoding utf8
if($LASTEXITCODE){throw 'Optimized configure failed; log retained'}
& $auditCmake --build $auditBuild --target bo1_stringtable_normalization_tests --verbose `
    2>&1 | Out-File (Join-Path $auditBuild 'build.log') -Encoding utf8
if($LASTEXITCODE){Get-Content (Join-Path $auditBuild 'build.log') -Tail 20;throw 'Optimized build failed; log retained'}
Write-Output "Optimized ARM64 with assertions/UBSan: $auditBuild"
