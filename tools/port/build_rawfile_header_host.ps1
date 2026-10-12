param([string]$PythonExe='C:/Users/recox/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe', [string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat', [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput=[IO.Path]::GetFullPath((Join-Path $probeRoot $OutputDirectory))
if (!$probeOutput.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Only Android worktree outputs allowed' }
if(Test-Path -LiteralPath $probeOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $probeOutput | Out-Null
$hashControl=Join-Path $probeOutput 'windows_original_hash.cpp'
& $PythonExe (Join-Path $probeRoot 'tools/port/generate_stringtable_hash_reference.py') $hashControl 2>&1 | Out-File (Join-Path $probeOutput 'reference-generation.log') -Encoding utf8
if($LASTEXITCODE){throw 'Historical Windows control generation failed; evidence retained'}
$freeControl=Join-Path $probeOutput 'windows_original_free.cpp'
& $PythonExe (Join-Path $probeRoot 'tools/port/generate_stringtable_free_reference.py') $freeControl 2>&1 | Out-File (Join-Path $probeOutput 'free-reference-generation.log') -Encoding utf8
if($LASTEXITCODE){throw 'Historical Windows free control generation failed; evidence retained'}
foreach($arch in @('x86','x64')){
    $output=Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $output | Out-Null
    $defines=if($arch -eq 'x64'){'/DBO1_PORT_X64=1'}else{''}
    $files=@('tools/port/tests/raw_strings_tests.cpp','tools/port/tests/stream_init_support.cpp',
        'tools/port/tests/checked_zone_support.cpp','src/platform/fastfile_script_table.cpp','src/platform/fastfile_raw_string.cpp','src/platform/fastfile_asset_directory.cpp','src/platform/fastfile_stringtable_header.cpp','src/platform/fastfile_stringtable_cells.cpp','src/platform/fastfile_initialized_strings.cpp','src/platform/fastfile_stringtable_indices.cpp','src/platform/fastfile_rawfile_header.cpp','src/database/db_load.cpp','src/database/db_registry.cpp','src/database/db_assetnames.cpp','src/universal/com_hash.cpp','src/universal/com_stringtable_obj.cpp','src/universal/com_stringtable.cpp','src/universal/q_shared.cpp','src/universal/com_stringtable_sort.cpp','src/platform/win32_compat/sort16.cpp','src/database/db_stream_load.cpp',
        'src/platform/fastfile_assetlist.cpp','src/platform/fastfile_zone.cpp','src/platform/fastfile_inflate.cpp',
        'src/database/db_expand.cpp','src/database/db_stream.cpp','src/database/db_memory.cpp','src/universal/physicalmemory.cpp',
        'src/zlib/inflate.c','src/zlib/inffast.c','src/zlib/inftrees.c','src/zlib/adler32.c','src/zlib/crc32.c','src/zlib/zutil.c')
    $sources=(($files|ForEach-Object{'"'+(Join-Path $probeRoot $_)+'"'}) -join ' ')+' "'+$hashControl+'" "'+$freeControl+'"'
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 '+$defines+
        ' /DBO1_ASSETLIST_GATE=1 /DBO1_FRONTEND_TABLES_GATE=1 /DBO1_FRONTEND_TRACE_LOGS=1 /DBO1_RAW_STRING_GATE=1 /DBO1_ASSET_DIRECTORY_GATE=1 /DBO1_STRINGTABLE_HEADER_GATE=1 /DBO1_STRINGTABLE_CELLS_GATE=1 /DBO1_STRINGTABLE_STRINGS_GATE=1 /DBO1_STRINGTABLE_INDEX_GATE=1 /DBO1_STRINGTABLE_HASH_GATE=1 /DBO1_STRINGTABLE_LOOKUP_GATE=1 /DBO1_STRINGTABLE_NORMALIZATION_GATE=1 /DBO1_STRINGTABLE_OWNER_GATE=1 /DBO1_RAWFILE_HEADER_GATE=1'+
        ' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" /I"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include" '+$sources+
        ' /Fe"'+(Join-Path $output 'rawfile_header.exe')+'" /Fo"'+$output+'/" /link /OPT:REF /MAP'
    $command|Out-File (Join-Path $output 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1|Out-File (Join-Path $output 'compile.log') -Encoding utf8
    $compileExit=$LASTEXITCODE
    @{architecture=$arch;exit=$compileExit;completed=($compileExit -eq 0)} | ConvertTo-Json |
        Out-File (Join-Path $output 'build-result.json') -Encoding utf8
    if($compileExit){Get-Content (Join-Path $output 'compile.log') -Tail 25;throw "$arch compile failed"}
}
Write-Output "Production Windows genuine RawFile header after StringTable pool ownership with historical hash/free controls: $probeOutput"
