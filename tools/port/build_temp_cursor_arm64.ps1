param([string]$SdkRoot='D:/Android/Sdk', [string]$StreamSource='', [Parameter(Mandatory=$true)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeBuild=[IO.Path]::GetFullPath((Join-Path $probeRoot $BuildDirectory))
if(!$probeBuild.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Only Android worktree outputs allowed'}
if(Test-Path -LiteralPath $probeBuild){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $probeBuild|Out-Null
$clang=Join-Path $SdkRoot 'ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$sysroot=Join-Path $SdkRoot 'ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot'
$arguments=@('--target=aarch64-none-linux-android28',"--sysroot=$sysroot",'-std=c++20','-O3','-g','-D_DEBUG=1','-DBO1_FRONTEND_TRACE_LOGS=1','-fms-extensions','-fsigned-char','-Wall','-Wextra','-Wpedantic','-Werror=return-type','-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all','-static-libstdc++','-include',(Join-Path $probeRoot 'src/platform/win32_compat/msvc_compat.h'),'-I',(Join-Path $probeRoot 'src'),'-I',$probeRoot)
foreach($source in @('tools/port/tests/temp_cursor_tests.cpp','tools/port/tests/stream_init_support.cpp','tools/port/tests/checked_zone_support.cpp','src/platform/android/database_diagnostics.cpp')){$arguments+=Join-Path $probeRoot $source}
if(!$StreamSource){$StreamSource=Join-Path $probeRoot 'src/database/db_stream.cpp'}
$resolvedStream=[IO.Path]::GetFullPath($StreamSource)
if(!$resolvedStream.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Stream source must remain inside Android worktree'}
$arguments+=$resolvedStream
$arguments+=@('-I',(Join-Path $probeRoot 'src/database'))
$arguments+=@('-Wl,--gc-sections','-latomic','-o',(Join-Path $probeBuild 'bo1_temp_cursor_tests'))
@{executable=$clang;argv=$arguments}|ConvertTo-Json -Depth 4|Out-File (Join-Path $probeBuild 'command.json') -Encoding utf8
& $clang @arguments 2>&1|Out-File (Join-Path $probeBuild 'build.log') -Encoding utf8
if($LASTEXITCODE){Get-Content (Join-Path $probeBuild 'build.log') -Tail 20;throw 'ARM64 compile failed; diagnostics retained'}
Write-Output "Optimized ARM64 TEMP cursor/alias prerequisite with assertions/UBSan: $probeBuild"
