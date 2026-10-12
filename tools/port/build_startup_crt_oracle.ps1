param([Parameter(Mandatory=$true)][string]$OutputDirectory,
      [switch]$NativeWrapper,
      [string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if(!$taskOutput.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Output must stay in Android worktree'}
if(Test-Path -LiteralPath $taskOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $taskOutput|Out-Null
$taskSource=Join-Path $PSScriptRoot 'startup_crt_oracle.cpp'
foreach($taskArch in @('x86','x64')){
    $taskArchOutput=Join-Path $taskOutput $taskArch
    New-Item -ItemType Directory -Path $taskArchOutput|Out-Null
    $taskBinary=Join-Path $taskArchOutput 'startup_crt_oracle.exe'
    $taskExtra=''
    if($NativeWrapper){$taskExtra=' /DBO1_STARTUP_CRT_NATIVE_ORACLE=1 /I"'+(Join-Path $taskRoot 'src')+'" "'+(Join-Path $taskRoot 'src/platform/android/legacy_crt.cpp')+'"'}
    $taskCommand='"'+$VcVars+'" '+$taskArch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MT /O2 "'+$taskSource+'"'+$taskExtra+' /Fe"'+$taskBinary+'" /Fo"'+$taskArchOutput+'/"'
    $taskCommand|Out-File (Join-Path $taskArchOutput 'command.txt') -Encoding utf8
    $ErrorActionPreference='Continue'
    & cmd /d /s /c $taskCommand *> (Join-Path $taskArchOutput 'compile.log')
    $ErrorActionPreference='Stop'
    if($LASTEXITCODE){throw 'CRT oracle compile failed'}
    $taskStdout=& $taskBinary
    $taskExit=$LASTEXITCODE
    $taskStdout|Out-File (Join-Path $taskArchOutput 'run.log') -Encoding utf8
    $taskResult=@{exit_code=$taskExit;native_wrapper=[bool]$NativeWrapper;source_sha256=(Get-FileHash -LiteralPath $taskSource -Algorithm SHA256).Hash;binary_sha256=(Get-FileHash -LiteralPath $taskBinary -Algorithm SHA256).Hash}
    if($NativeWrapper){$taskResult.wrapper_sha256=(Get-FileHash -LiteralPath (Join-Path $taskRoot 'src/platform/android/legacy_crt.cpp') -Algorithm SHA256).Hash}
    $taskResult|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'result.json') -Encoding utf8
    if($taskExit){throw "CRT oracle failed: $taskExit"}
}
