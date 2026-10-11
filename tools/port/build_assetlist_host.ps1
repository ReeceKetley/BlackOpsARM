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
    $files=@('tools/port/tests/assetlist_tests.cpp','tools/port/tests/stream_init_support.cpp',
        'tools/port/tests/checked_zone_support.cpp','src/platform/fastfile_zone.cpp','src/platform/fastfile_assetlist.cpp',
        'src/platform/fastfile_inflate.cpp','src/database/db_expand.cpp','src/database/db_memory.cpp',
        'src/database/db_stream.cpp','src/universal/physicalmemory.cpp','src/zlib/inflate.c','src/zlib/inffast.c',
        'src/zlib/inftrees.c','src/zlib/adler32.c','src/zlib/crc32.c','src/zlib/zutil.c')
    $sources=($files|ForEach-Object{'"'+(Join-Path $probeRoot $_)+'"'}) -join ' '
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 /DBO1_ASSETLIST_GATE=1 '+$defines+
        ' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" '+$sources+' /Fe"'+(Join-Path $archOutput 'assetlist.exe')+'" /Fo"'+$archOutput+'/" /link /OPT:REF /MAP'
    $command|Out-File (Join-Path $archOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1 |Out-File (Join-Path $archOutput 'compile.log') -Encoding utf8
    if($LASTEXITCODE){Get-Content (Join-Path $archOutput 'compile.log') -Tail 25;throw "$arch compile failed"}
}
Write-Output "Production Windows envelope binaries: $probeOutput"
