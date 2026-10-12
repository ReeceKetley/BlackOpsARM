param([string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat', [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput=[IO.Path]::GetFullPath((Join-Path $probeRoot $OutputDirectory))
if (!$probeOutput.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Only Android worktree outputs allowed' }
if(Test-Path -LiteralPath $probeOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach($arch in @('x86','x64')){
    $output=Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $output | Out-Null
    $defines=if($arch -eq 'x64'){'/DBO1_PORT_X64=1'}else{''}
    $files=@('tools/port/tests/raw_strings_tests.cpp','tools/port/tests/string_pool_support.cpp','src/clientscript/cscr_stringlist.cpp','src/clientscript/cscr_memorytree.cpp','src/clientscript/cscr_instance.cpp','src/platform/android/script_pool_bridge.cpp','src/platform/android/script_string_services.cpp','tools/port/tests/stream_init_support.cpp',
        'tools/port/tests/checked_zone_support.cpp','src/platform/fastfile_script_table.cpp','src/platform/fastfile_raw_string.cpp','src/database/db_stream_load.cpp',
        'src/platform/fastfile_assetlist.cpp','src/platform/fastfile_zone.cpp','src/platform/fastfile_inflate.cpp',
        'src/database/db_expand.cpp','src/database/db_stream.cpp','src/database/db_memory.cpp','src/universal/physicalmemory.cpp',
        'src/zlib/inflate.c','src/zlib/inffast.c','src/zlib/inftrees.c','src/zlib/adler32.c','src/zlib/crc32.c','src/zlib/zutil.c')
    $sources=($files|ForEach-Object{'"'+(Join-Path $probeRoot $_)+'"'}) -join ' '
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 '+$defines+
        ' /DBO1_ASSETLIST_GATE=1 /DBO1_FRONTEND_TABLES_GATE=1 /DBO1_FRONTEND_TRACE_LOGS=1 /DBO1_RAW_STRING_GATE=1 /DBO1_STRING_POOL_GATE=1 /DBO1_FRONTEND_INTERN_GATE=1 /DBO1_SCRIPT_STRING_SERVICES_HOST_TEST=1'+
        ' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" '+$sources+
        ' /Fe"'+(Join-Path $output 'intern_strings.exe')+'" /Fo"'+$output+'/" /link /OPT:REF /MAP'
    $command|Out-File (Join-Path $output 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1|Out-File (Join-Path $output 'compile.log') -Encoding utf8
    if($LASTEXITCODE){Get-Content (Join-Path $output 'compile.log') -Tail 25;throw "$arch compile failed"}
}
Write-Output "Actual Windows genuine interning: $probeOutput"
