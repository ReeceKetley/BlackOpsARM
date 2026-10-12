param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if (!$taskOutput.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {throw 'Output must stay in Android worktree'}
if(Test-Path -LiteralPath $taskOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $taskOutput|Out-Null
$taskFiles=@('tools/port/tests/pmem_owner_stack_tests.cpp','tools/port/tests/checked_zone_support.cpp','src/universal/physicalmemory.cpp')
foreach($taskArch in @('x86','x64')){
    $taskArchOutput=Join-Path $taskOutput $taskArch;New-Item -ItemType Directory -Path $taskArchOutput|Out-Null
    $taskDefines=if($taskArch -eq 'x64'){'/DBO1_PORT_X64=1'}else{''}
    $taskHashes=@{}
    foreach($unit in ($taskFiles+@('src/universal/physicalmemory.h','src/platform/bo1_native_width.h','tools/port/build_pmem_owner_stack_host.ps1'))){$taskHashes[$unit]=(Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash}
    $taskHashes|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'source-manifest.json') -Encoding utf8
    $taskSources=($taskFiles|ForEach-Object{'"'+(Join-Path $taskRoot $_)+'"'}) -join ' '
    $taskCommand='"'+$VcVars+'" '+$taskArch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 '+$taskDefines+' /I"'+(Join-Path $taskRoot 'src')+'" /I"'+$taskRoot+'" '+$taskSources+' /Fe"'+(Join-Path $taskArchOutput 'pmem_owner_stack.exe')+'" /Fo"'+$taskArchOutput+'/" /link /OPT:REF'
    $taskCommand|Out-File (Join-Path $taskArchOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $taskCommand *> (Join-Path $taskArchOutput 'compile.log')
    $taskExit=$LASTEXITCODE
    foreach($unit in $taskHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $taskRoot $unit) -Algorithm SHA256).Hash -ne $taskHashes[$unit]){throw 'Source changed during build'}}
    @{architecture=$taskArch;exit=$taskExit;completed=($taskExit -eq 0)}|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'build-result.json') -Encoding utf8
    if($taskExit){Get-Content (Join-Path $taskArchOutput 'compile.log') -Tail 20;throw 'PMem test build failed'}
    & (Join-Path $taskArchOutput 'pmem_owner_stack.exe') *> (Join-Path $taskArchOutput 'run.log')
    @{architecture=$taskArch;exit=$LASTEXITCODE}|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'run-result.json') -Encoding utf8
}
