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
    $files=@('tools/port/tests/native_zone_file_tests.cpp','tools/port/tests/stream_init_support.cpp',
        'tools/port/tests/checked_zone_support.cpp','src/platform/fastfile_zone.cpp',
        'src/platform/native_zone_file.cpp','src/platform/fastfile_inflate.cpp',
        'src/database/db_memory.cpp','src/database/db_stream.cpp','src/universal/physicalmemory.cpp',
        'src/zlib/inflate.c','src/zlib/inffast.c','src/zlib/inftrees.c','src/zlib/adler32.c','src/zlib/crc32.c','src/zlib/zutil.c')
    $sources=($files|ForEach-Object{'"'+(Join-Path $probeRoot $_)+'"'}) -join ' '
    $nativeHashes=@{}
    foreach($unit in ($files+@('src/platform/native_zone_file.h','src/platform/fastfile_inflate.h','src/platform/fastfile_zone.h','src/database/db_stream.h','src/database/db_zone_types.h','src/platform/bo1_native_width.h','tools/port/build_native_zone_file_host.ps1'))) {
        $nativeHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $probeRoot $unit) -Algorithm SHA256).Hash
    }
    $nativeHashes|ConvertTo-Json -Depth 3|Out-File (Join-Path $archOutput 'source-manifest.json') -Encoding utf8
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 '+$defines+
        ' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" '+$sources+' /Fe"'+(Join-Path $archOutput 'native_zone_file.exe')+'" /Fo"'+$archOutput+'/" /link /OPT:REF'
    $command|Out-File (Join-Path $archOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1 |Out-File (Join-Path $archOutput 'compile.log') -Encoding utf8
    if($LASTEXITCODE){Get-Content (Join-Path $archOutput 'compile.log') -Tail 25;throw "$arch compile failed"}
    foreach($unit in $nativeHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $probeRoot $unit) -Algorithm SHA256).Hash -ne $nativeHashes[$unit]){throw 'Native reader source changed during build'}}
    @{completed=$true;build_exit=0;architecture=$arch}|ConvertTo-Json|Out-File (Join-Path $archOutput 'build-result.json') -Encoding utf8
}
Write-Output "Production Windows native zone file binaries: $probeOutput"
