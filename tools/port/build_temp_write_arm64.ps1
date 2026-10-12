param([string]$SdkRoot='D:/Android/Sdk', [string]$StreamSource='', [Parameter(Mandatory=$true)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeBuild=[IO.Path]::GetFullPath((Join-Path $probeRoot $BuildDirectory))
if(!$probeBuild.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Only Android worktree outputs allowed'}
if(Test-Path -LiteralPath $probeBuild){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $probeBuild|Out-Null
$clang=Join-Path $SdkRoot 'ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
$sysroot=Join-Path $SdkRoot 'ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot'
$arguments=@('--target=aarch64-none-linux-android28',"--sysroot=$sysroot",'-std=c++20','-O3','-g','-D_DEBUG=1','-DBO1_ASSETLIST_GATE=1','-DBO1_STRINGTABLE_HEADER_GATE=1','-DBO1_RAWFILE_HEADER_GATE=1','-DBO1_RAW_STRING_GATE=1','-DBO1_FRONTEND_TRACE_LOGS=1','-fms-extensions','-fsigned-char','-Wall','-Wextra','-Wpedantic','-Werror=return-type','-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all','-static-libstdc++','-include',(Join-Path $probeRoot 'src/platform/win32_compat/msvc_compat.h'),'-I',(Join-Path $probeRoot 'src'),'-I',$probeRoot)
foreach($source in @('tools/port/tests/temp_write_tests.cpp','tools/port/tests/stream_init_support.cpp','tools/port/tests/checked_zone_support.cpp','src/platform/android/database_diagnostics.cpp','src/database/db_stream_load.cpp','src/database/db_expand.cpp')){$arguments+=Join-Path $probeRoot $source}
if(!$StreamSource){$StreamSource=Join-Path $probeRoot 'src/database/db_stream.cpp'}
$resolvedStream=[IO.Path]::GetFullPath($StreamSource)
if(!$resolvedStream.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Stream source must remain inside Android worktree'}
$arguments+=$resolvedStream
$arguments+=@('-I',(Join-Path $probeRoot 'src/database'))
$arguments+=@('-Wl,--gc-sections','-latomic','-o',(Join-Path $probeBuild 'bo1_temp_write_tests'))
$inputHashes=@{}
foreach($unit in (@('tools/port/tests/temp_write_tests.cpp','tools/port/tests/stream_init_support.cpp','tools/port/tests/checked_zone_support.cpp','src/platform/android/database_diagnostics.cpp','src/database/db_stream.cpp','src/database/db_stream_load.cpp','src/database/db_expand.cpp')+@('src/database/db_stream.h','src/database/db_stream_load.h','src/database/db_expand.hpp','src/database/db_rawfile_types.h','src/database/db_zone_types.h','src/universal/com_stringtable.h','src/platform/bo1_native_width.h','src/platform/win32_compat/msvc_compat.h'))){$inputHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $probeRoot $unit) -Algorithm SHA256).Hash}
$inputHashes[$PSCommandPath]=(Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
$inputHashes|ConvertTo-Json -Depth 4|Out-File (Join-Path $probeBuild 'source-manifest.json') -Encoding utf8
@{executable=$clang;argv=$arguments}|ConvertTo-Json -Depth 4|Out-File (Join-Path $probeBuild 'command.json') -Encoding utf8
& $clang @arguments 2>&1|Out-File (Join-Path $probeBuild 'build.log') -Encoding utf8
$compileExit=$LASTEXITCODE
foreach($unit in $inputHashes.Keys){$absolute=if([IO.Path]::IsPathRooted($unit)){$unit}else{Join-Path $probeRoot $unit};if((Get-FileHash -LiteralPath $absolute -Algorithm SHA256).Hash -ne $inputHashes[$unit]){throw 'Source changed during compile; logs preserved'}}
$LASTEXITCODE=$compileExit
if($LASTEXITCODE){Get-Content (Join-Path $probeBuild 'build.log') -Tail 20;throw 'ARM64 compile failed; diagnostics retained'}
Write-Output "Optimized ARM64 TEMP write/mapping prerequisite with assertions/UBSan: $probeBuild"
