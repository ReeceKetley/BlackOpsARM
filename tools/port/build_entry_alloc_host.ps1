param([string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat',[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if(!$taskOutput.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)-or(Test-Path -LiteralPath $taskOutput)){throw 'Fresh Android-worktree output required'}
New-Item -ItemType Directory -Path $taskOutput | Out-Null
$taskSources=@('tools/port/tests/entry_alloc_tests.cpp','src/database/db_registry.cpp','src/database/db_assetnames.cpp','src/platform/android/script_pool_bridge.cpp','src/platform/android/script_string_services.cpp')
$taskHeaders=@('src/database/db_entry_alloc_api.h','src/database/db_zone_name_types.h','src/universal/dvar.h','src/universal/com_errors.h','src/database/db_asset_entry_types.h','src/database/db_asset_pool.h','src/database/db_assetlist_types.h','src/database/database.h','src/database/db_rawfile_types.h','src/universal/com_stringtable.h','src/universal/assertive.h','src/universal/critical_sections.h','src/platform/critical_section_services.h','src/platform/android/script_string_services.h','src/platform/bo1_native_width.h','src/platform/win32_compat/msvc_compat.h')
$taskManifest=@{}
foreach($unit in $taskSources+$taskHeaders){$taskManifest[$unit]=(Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash}
$taskManifest[$PSCommandPath]=(Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
foreach($arch in @('x86','x64')){
 $taskArch=Join-Path $taskOutput $arch;New-Item -ItemType Directory -Path $taskArch | Out-Null
 $taskManifest | ConvertTo-Json | Out-File (Join-Path $taskArch 'source-manifest.json') -Encoding utf8
 $taskDefines=if($arch -eq 'x64'){' /DBO1_PORT_X64=1'}else{''}
 $taskInputs=($taskSources|ForEach-Object{'"'+(Join-Path $taskRoot $_)+'"'}) -join ' '
 $taskCommand='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 /DBO1_STRINGTABLE_OWNER_GATE=1 /DBO1_ASSET_ENTRY_GATE=1 /DBO1_ENTRY_ALLOC_GATE=1 /DBO1_RAWFILE_OWNER_GATE=1 /DBO1_SCRIPT_STRING_SERVICES_HOST_TEST=1'+$taskDefines+' /I"'+(Join-Path $taskRoot 'src')+'" '+$taskInputs+' /Fe"'+(Join-Path $taskArch 'entry_alloc.exe')+'" /Fo"'+$taskArch+'/" /link /OPT:REF /MAP'
 $taskCommand | Out-File (Join-Path $taskArch 'command.txt') -Encoding utf8
 & cmd /d /s /c $taskCommand 2>&1 | Out-File (Join-Path $taskArch 'compile.log') -Encoding utf8
 $taskExit=$LASTEXITCODE
 @{architecture=$arch;exit=$taskExit;completed=($taskExit -eq 0)}|ConvertTo-Json|Out-File (Join-Path $taskArch 'build-result.json') -Encoding utf8
 if($taskExit){Get-Content (Join-Path $taskArch 'compile.log') -Tail 20;throw 'Host compilation failed; logs retained'}
}
foreach($unit in $taskManifest.Keys){$path=if([IO.Path]::IsPathRooted($unit)){$unit}else{Join-Path $taskRoot $unit};if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $taskManifest[$unit]){throw 'Input changed during compilation'}}
Write-Output "Windows x86/x64 canonical entry initialization: $taskOutput"
