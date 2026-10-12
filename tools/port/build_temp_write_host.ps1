param([string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat', [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput=[IO.Path]::GetFullPath((Join-Path $probeRoot $OutputDirectory))
if (!$probeOutput.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Build only in Android worktree' }
if (Test-Path -LiteralPath $probeOutput) { throw 'Use fresh output directory' }
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach($arch in @('x86','x64')) {
    $archOutput=Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $archOutput | Out-Null
    $defines=if($arch -eq 'x64'){'/DBO1_PORT_X64=1'}else{''}
    $files=@('tools/port/tests/temp_write_tests.cpp','tools/port/tests/stream_init_support.cpp',
        'tools/port/tests/checked_zone_support.cpp','src/platform/fastfile_zone.cpp',
        'src/database/db_memory.cpp','src/database/db_stream.cpp','src/database/db_stream_load.cpp','src/database/db_expand.cpp','src/universal/physicalmemory.cpp')
    $inputHashes=@{}
    foreach($unit in ($files+@('src/database/db_stream.h','src/database/db_stream_load.h','src/database/db_expand.hpp','src/database/db_rawfile_types.h','src/database/db_zone_types.h','src/universal/com_stringtable.h','src/platform/bo1_native_width.h','src/platform/win32_compat/msvc_compat.h'))){$inputHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $probeRoot $unit) -Algorithm SHA256).Hash}
    $inputHashes[$PSCommandPath]=(Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
    $inputHashes|ConvertTo-Json -Depth 4|Out-File (Join-Path $archOutput 'source-manifest.json') -Encoding utf8
    $sources=($files|ForEach-Object{'"'+(Join-Path $probeRoot $_)+'"'}) -join ' '
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 /DBO1_ASSETLIST_GATE=1 /DBO1_STRINGTABLE_HEADER_GATE=1 /DBO1_RAWFILE_HEADER_GATE=1 /DBO1_RAW_STRING_GATE=1 /DBO1_FRONTEND_TRACE_LOGS=1 '+$defines+
        ' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" '+$sources+' /Fe"'+(Join-Path $archOutput 'temp_write.exe')+'" /Fo"'+$archOutput+'/" /link /OPT:REF'
    $command|Out-File (Join-Path $archOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1 |Out-File (Join-Path $archOutput 'compile.log') -Encoding utf8
    $compileExit=$LASTEXITCODE
    foreach($unit in $inputHashes.Keys){$absolute=if([IO.Path]::IsPathRooted($unit)){$unit}else{Join-Path $probeRoot $unit};if((Get-FileHash -LiteralPath $absolute -Algorithm SHA256).Hash -ne $inputHashes[$unit]){throw 'Source changed during compile; logs preserved'}}
    $LASTEXITCODE=$compileExit
    if($LASTEXITCODE){Get-Content (Join-Path $archOutput 'compile.log') -Tail 25;throw "$arch compile failed"}
}
Write-Output "Production Windows TEMP write/mapping prerequisite binaries: $probeOutput"
