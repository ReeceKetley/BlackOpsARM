param([Parameter(Mandatory=$true)][string]$OracleDirectory,[Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOracle=[IO.Path]::GetFullPath((Join-Path $taskRoot $OracleDirectory))
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
foreach($path in @($taskOracle,$taskOutput)){if(!$path.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Oracle/output must stay in Android worktree'}}
if(Test-Path -LiteralPath $taskOutput){throw 'Use fresh output directory'}
$taskSource=Join-Path $taskOracle 'original_fast_critical_section.cpp'
$taskProvenance=Get-Content (Join-Path $taskOracle 'oracle.json') -Raw|ConvertFrom-Json
if((Get-FileHash -LiteralPath $taskSource -Algorithm SHA256).Hash -ne $taskProvenance.generated_cpp_sha256){throw 'Original oracle source changed'}
New-Item -ItemType Directory -Path $taskOutput|Out-Null
foreach($taskArch in @('x86','x64')){
    $taskArchOutput=Join-Path $taskOutput $taskArch;New-Item -ItemType Directory -Path $taskArchOutput|Out-Null
    $taskBinary=Join-Path $taskArchOutput 'original_fast_critical_section.exe'
    $taskCommand='"'+$VcVars+'" '+$taskArch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MT /O2 "'+$taskSource+'" /Fe"'+$taskBinary+'" /Fo"'+$taskArchOutput+'/"'
    $taskCommand|Out-File (Join-Path $taskArchOutput 'command.txt') -Encoding utf8
    & cmd /d /s /c $taskCommand *> (Join-Path $taskArchOutput 'compile.log')
    $taskExit=$LASTEXITCODE
    @{architecture=$taskArch;exit=$taskExit;completed=($taskExit -eq 0);original_cpp_sha256=$taskProvenance.generated_cpp_sha256}|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'build-result.json') -Encoding utf8
    if($taskExit){Get-Content (Join-Path $taskArchOutput 'compile.log') -Tail 20;throw 'Original FastCriticalSection build failed'}
    & $taskBinary *> (Join-Path $taskArchOutput 'run.log')
    $taskExit=$LASTEXITCODE
    @{architecture=$taskArch;exit=$taskExit;binary_sha256=(Get-FileHash -LiteralPath $taskBinary -Algorithm SHA256).Hash}|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'run-result.json') -Encoding utf8
    if($taskExit){Get-Content (Join-Path $taskArchOutput 'run.log') -Tail 20;throw 'Original FastCriticalSection runtime failed'}
}
