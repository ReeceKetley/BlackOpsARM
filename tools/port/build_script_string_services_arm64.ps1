param([Parameter(Mandatory=$true)][string]$BuildDirectory,[string]$SdkRoot='D:/Android/Sdk')
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeBuild=[IO.Path]::GetFullPath((Join-Path $probeRoot $BuildDirectory))
if(!$probeBuild.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Only Android worktree outputs allowed'}
if(Test-Path -LiteralPath $probeBuild){throw 'Use a fresh build directory'}
New-Item -ItemType Directory -Path $probeBuild|Out-Null
$probeCmake=Join-Path $SdkRoot 'cmake/3.22.1/bin/cmake.exe'
$probeNinja=Join-Path $SdkRoot 'cmake/3.22.1/bin/ninja.exe'
$probeToolchain=Join-Path $SdkRoot 'ndk/30.0.16248370/build/cmake/android.toolchain.cmake'
& $probeCmake -S (Join-Path $probeRoot 'android/cmake/arm64-audit') -B $probeBuild -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$probeNinja" "-DCMAKE_TOOLCHAIN_FILE=$probeToolchain" `
    '-DANDROID_ABI=arm64-v8a' '-DANDROID_PLATFORM=android-28' '-DCMAKE_BUILD_TYPE=Debug' '-DBO1_ARM64_COMPAT=ON' `
    2>&1|Out-File (Join-Path $probeBuild 'configure.log') -Encoding utf8
if($LASTEXITCODE){throw 'Service configure failed; log retained'}
& $probeCmake --build $probeBuild --target bo1_script_string_services_tests --verbose `
    2>&1|Out-File (Join-Path $probeBuild 'build.log') -Encoding utf8
if($LASTEXITCODE){Get-Content (Join-Path $probeBuild 'build.log') -Tail 20;throw 'Service build failed; log retained'}
Write-Output "PASS ARM64 real synchronization prerequisites: $probeBuild"
