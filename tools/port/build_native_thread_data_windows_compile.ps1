param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if (!$taskOutput.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {throw 'Output must stay in Android worktree'}
if(Test-Path -LiteralPath $taskOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $taskOutput|Out-Null
$taskFiles=@('src/universal/q_shared.cpp','src/qcommon/cmd.cpp','src/qcommon/threads.cpp','src/qcommon/cm_trace.cpp')
foreach($taskArch in @('x86','x64')){
    $taskArchOutput=Join-Path $taskOutput $taskArch;New-Item -ItemType Directory -Path $taskArchOutput|Out-Null
    $taskDefines=if($taskArch -eq 'x64'){'/DBO1_PORT_X64=1'}else{''}
    $taskHashes=@{}
    foreach($unit in ($taskFiles+@('src/qcommon/cmd.h','src/qcommon/cmd_args.h','src/qcommon/cm_trace.h','src/qcommon/trace_thread_info.h','src/qcommon/threads.h','src/qcommon/thread_context.h','src/universal/thread_data.h','tools/port/build_native_thread_data_windows_compile.ps1'))){$taskHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash}
    $taskHashes|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'source-manifest.json') -Encoding utf8
    $taskSources=($taskFiles|ForEach-Object{'"'+(Join-Path $taskRoot $_)+'"'}) -join ' '
    $taskCommand='"'+$VcVars+'" '+$taskArch+' >nul && cl /nologo /W3 /std:c++20 /EHsc /MTd /O2 /D_DEBUG=1 '+$taskDefines+' /I"'+(Join-Path $taskRoot 'src')+'" /I"'+$taskRoot+'" /I"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include" /c '+$taskSources+' /Fo"'+$taskArchOutput+'/"'
    $taskCommand|Out-File (Join-Path $taskArchOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $taskCommand *> (Join-Path $taskArchOutput 'compile.log')
    $taskExit=$LASTEXITCODE
    foreach($unit in $taskHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash -ne $taskHashes[$unit]){throw 'Source changed during normal build'}}
    @{architecture=$taskArch;exit=$taskExit;completed=($taskExit -eq 0);native_selection=$false;translation_units=$taskFiles.Count}|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'build-result.json') -Encoding utf8
    if($taskExit){Get-Content (Join-Path $taskArchOutput 'compile.log') -Tail 24;throw 'Normal Windows thread-data closure build failed'}
}
