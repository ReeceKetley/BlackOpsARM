param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat',
    [Parameter(Mandatory=$true)][string]$StartupServicesBuildDirectory)
$ErrorActionPreference='Stop'
$nativeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$nativeOutput=[IO.Path]::GetFullPath((Join-Path $nativeRoot $OutputDirectory))
if (!$nativeOutput.StartsWith($nativeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Only Android worktree output allowed' }
if(Test-Path -LiteralPath $nativeOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $nativeOutput|Out-Null
$nativeFiles=@('tools/port/tests/native_startup_services_zone_tests.cpp','tools/port/tests/native_startup_zone_tests.cpp','tools/port/tests/entry_owner_support.cpp','tools/port/tests/checked_zone_support.cpp',
    'src/platform/android/script_pool_bridge.cpp','src/platform/android/script_string_services.cpp',
    'src/platform/native_zone_load.cpp','src/platform/native_zone_file.cpp','src/platform/fastfile_assetlist.cpp','src/platform/fastfile_asset_directory.cpp',
    'src/platform/fastfile_stringtable_header.cpp','src/platform/fastfile_stringtable_cells.cpp','src/platform/fastfile_stringtable_indices.cpp',
    'src/platform/fastfile_raw_string.cpp','src/platform/fastfile_initialized_strings.cpp','src/platform/fastfile_rawfile_header.cpp','src/platform/fastfile_rawfile_payload.cpp','src/platform/fastfile_zone.cpp','src/platform/fastfile_inflate.cpp',
    'src/database/db_file_load.cpp','src/database/db_load.cpp','src/database/db_registry.cpp','src/database/db_assetnames.cpp','src/database/db_expand.cpp','src/database/db_stream.cpp','src/database/db_stream_load.cpp','src/database/db_memory.cpp','src/universal/physicalmemory.cpp',
    'src/universal/com_stringtable.cpp','src/universal/com_stringtable_obj.cpp','src/universal/com_stringtable_sort.cpp','src/universal/com_hash.cpp','src/platform/win32_compat/sort16.cpp',
    'src/zlib/inflate.c','src/zlib/inffast.c','src/zlib/inftrees.c','src/zlib/adler32.c','src/zlib/crc32.c','src/zlib/zutil.c')
$nativeFlags=@('BO1_NATIVE_ZONE_LOAD','BO1_ASSETLIST_GATE','BO1_FRONTEND_TABLES_GATE','BO1_RAW_STRING_GATE','BO1_ASSET_DIRECTORY_GATE','BO1_STRINGTABLE_HEADER_GATE','BO1_STRINGTABLE_CELLS_GATE','BO1_STRINGTABLE_INDEX_GATE','BO1_STRINGTABLE_HASH_GATE','BO1_STRINGTABLE_LOOKUP_GATE','BO1_STRINGTABLE_NORMALIZATION_GATE','BO1_STRINGTABLE_OWNER_GATE','BO1_RAWFILE_HEADER_GATE','BO1_RAWFILE_PAYLOAD_GATE','BO1_RAWFILE_OWNER_GATE','BO1_ASSET_ENTRY_GATE','BO1_ENTRY_ALLOC_GATE','BO1_REGISTRY_LOOKUP_GATE','BO1_REGISTRY_INSERT_GATE','BO1_REGISTRY_ADD_GATE','BO1_SCRIPT_STRING_SERVICES_HOST_TEST')
foreach($nativeArch in @('x86','x64')){
    $nativeArchOutput=Join-Path $nativeOutput $nativeArch;New-Item -ItemType Directory -Path $nativeArchOutput|Out-Null
    $nativeDefines=($nativeFlags|ForEach-Object{'/D'+$_+'=1'}) -join ' '
    if($nativeArch -eq 'x64'){$nativeDefines+=' /DBO1_PORT_X64=1'}
    $nativeServiceBuild=[IO.Path]::GetFullPath((Join-Path $nativeRoot $StartupServicesBuildDirectory))
    if(!$nativeServiceBuild.StartsWith($nativeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Service library must stay in Android worktree'}
    $nativeServiceArch=Join-Path $nativeServiceBuild $nativeArch
    $nativeServiceManifestPath=Join-Path $nativeServiceArch 'source-manifest.json'
    $nativeServiceManifest=Get-Content $nativeServiceManifestPath -Raw|ConvertFrom-Json
    $nativeServiceResult=Get-Content (Join-Path $nativeServiceArch 'build-result.json') -Raw|ConvertFrom-Json
    if(!$nativeServiceResult.completed -or $nativeServiceResult.exit -ne 0 -or $nativeServiceResult.architecture -ne $nativeArch -or $nativeServiceResult.original_windows_policy){throw 'Require passing native-policy startup library'}
    foreach($property in $nativeServiceManifest.PSObject.Properties){if((Get-FileHash -LiteralPath (Join-Path $nativeRoot $property.Name) -Algorithm SHA256).Hash -ne $property.Value){throw 'Startup library source stale'}}
    $nativeServiceLibrary=Join-Path $nativeServiceArch 'native_startup_services.lib'
    $nativeServiceHashes=@{}
    foreach($artifact in @($nativeServiceLibrary,$nativeServiceManifestPath)){$nativeServiceHashes[$artifact]=(Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash}
    @{architecture=$nativeArch;library=$nativeServiceLibrary;sha256=$nativeServiceHashes;source_manifest=$nativeServiceManifest}|ConvertTo-Json -Depth 8|Out-File (Join-Path $nativeArchOutput 'startup-library-provenance.json') -Encoding utf8
    $nativeDefines+=' /DBO1_STARTUP_SERVICES=1 /DBO1_STARTUP_SERVICES_ZONE_INTEGRATION=1 /DBO1_FAST_CRIT_SECT_NATIVE_HOST_TEST=1 /DBO1_STARTUP_SERVICES_NATIVE_HOST_TEST=1'
    $nativeHashes=@{}
    foreach($unit in ($nativeFiles+@('src/platform/native_zone_load.h','src/database/db_native_zone_load.h','tools/port/build_native_startup_services_zone_host.ps1'))){$nativeHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $nativeRoot $unit) -Algorithm SHA256).Hash}
    foreach($property in $nativeServiceManifest.PSObject.Properties){$nativeHashes[$property.Name]=$property.Value}
    foreach($artifact in $nativeServiceHashes.Keys){$nativeHashes[$artifact]=$nativeServiceHashes[$artifact]}
    $nativeHashes|ConvertTo-Json -Depth 3|Out-File (Join-Path $nativeArchOutput 'source-manifest.json') -Encoding utf8
    $nativeSources=($nativeFiles|ForEach-Object{'"'+(Join-Path $nativeRoot $_)+'"'}) -join ' '
    $nativeCommand='"'+$VcVars+'" '+$nativeArch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 '+$nativeDefines+
        ' /I"'+(Join-Path $nativeRoot 'src')+'" /I"'+$nativeRoot+'" /I"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include" '+$nativeSources+
        ' /Fe"'+(Join-Path $nativeArchOutput 'native_startup_services_zone.exe')+'" /Fo"'+$nativeArchOutput+'/" /link /OPT:REF /MAP '+('"'+$nativeServiceLibrary+'"')
    $nativeCommand|Out-File (Join-Path $nativeArchOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $nativeCommand *> (Join-Path $nativeArchOutput 'compile.log')
    $nativeExit=$LASTEXITCODE
    foreach($unit in $nativeHashes.Keys){$nativeInput=if([IO.Path]::IsPathRooted($unit)){$unit}else{Join-Path $nativeRoot $unit};if((Get-FileHash -LiteralPath $nativeInput -Algorithm SHA256).Hash -ne $nativeHashes[$unit]){throw 'Source/object changed during native startup build'}}
    @{architecture=$nativeArch;exit=$nativeExit;completed=($nativeExit -eq 0)}|ConvertTo-Json|Out-File (Join-Path $nativeArchOutput 'build-result.json') -Encoding utf8
    if($nativeExit){Get-Content (Join-Path $nativeArchOutput 'compile.log') -Tail 30;throw "$nativeArch startup build failed; evidence preserved"}
}
