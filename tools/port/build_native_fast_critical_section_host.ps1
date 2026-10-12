param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if (!$taskOutput.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {throw 'Output must stay in Android worktree'}
if(Test-Path -LiteralPath $taskOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $taskOutput|Out-Null
$taskFiles=@('tools/port/tests/native_fast_critical_section_tests.cpp','src/platform/android/fast_critical_section_services.cpp','src/database/db_registry.cpp','src/universal/dvar.cpp')
foreach($taskArch in @('x86','x64')){
    $taskArchOutput=Join-Path $taskOutput $taskArch;New-Item -ItemType Directory -Path $taskArchOutput|Out-Null
    $taskHashes=@{}
    foreach($unit in ($taskFiles+@('src/platform/fast_critical_section.h','src/platform/atomic32.h','src/win32/win_common.h','src/game_sp/g_sp_savegame.cpp','src/live/live_pcache.cpp','tools/port/build_native_fast_critical_section_host.ps1'))){$taskHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash}
    $taskHashes|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'source-manifest.json') -Encoding utf8
    $taskSources=($taskFiles|ForEach-Object{'"'+(Join-Path $taskRoot $_)+'"'}) -join ' '
    $taskCommand='"'+$VcVars+'" '+$taskArch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /D_DEBUG=1 /DBO1_FAST_CRIT_SECT_GATE=1 /DBO1_FAST_CRIT_SECT_NATIVE_HOST_TEST=1 /I"'+(Join-Path $taskRoot 'src')+'" '+$taskSources+' /Fe"'+(Join-Path $taskArchOutput 'native_fast_critical_section.exe')+'" /Fo"'+$taskArchOutput+'/"'
    $taskCommand|Out-File (Join-Path $taskArchOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $taskCommand *> (Join-Path $taskArchOutput 'compile.log')
    $taskExit=$LASTEXITCODE
    foreach($unit in $taskHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash -ne $taskHashes[$unit]){throw 'Source changed during build'}}
    @{architecture=$taskArch;exit=$taskExit;completed=($taskExit -eq 0)}|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'build-result.json') -Encoding utf8
    if($taskExit){Get-Content (Join-Path $taskArchOutput 'compile.log') -Tail 20;throw 'FastCriticalSection build failed'}
}
