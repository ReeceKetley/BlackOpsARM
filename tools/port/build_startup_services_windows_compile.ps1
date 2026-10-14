param([Parameter(Mandatory=$true)][string]$OutputDirectory,
      [string[]]$TranslationUnits=@('src/qcommon/common.cpp'),
      [string[]]$AdditionalCompilerOptions=@(),
      [string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if(!$taskOutput.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Output must stay in Android worktree'}
if(Test-Path -LiteralPath $taskOutput){throw 'Use fresh output directory'}
foreach($taskUnit in $TranslationUnits){
    $taskPath=[IO.Path]::GetFullPath((Join-Path $taskRoot $taskUnit))
    if(!$taskPath.StartsWith($taskRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Source must stay in Android worktree'}
}
New-Item -ItemType Directory -Path $taskOutput|Out-Null
foreach($taskArch in @('x86','x64')){
    $taskArchOutput=Join-Path $taskOutput $taskArch
    New-Item -ItemType Directory -Path $taskArchOutput|Out-Null
    $taskDefines=if($taskArch -eq 'x64'){'/DBO1_PORT_X64=1'}else{''}
    $taskHashes=@{}
    foreach($taskUnit in ($TranslationUnits+@('tools/port/build_startup_services_windows_compile.ps1'))){$taskHashes[$taskUnit]=(Get-FileHash -LiteralPath (Join-Path $taskRoot $taskUnit) -Algorithm SHA256).Hash}
    $taskHashes|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'source-manifest.json') -Encoding utf8
    $taskSources=($TranslationUnits|ForEach-Object{'"'+(Join-Path $taskRoot $_)+'"'}) -join ' '
    $taskCommand='"'+$VcVars+'" '+$taskArch+' >nul && cl /nologo /W3 /std:c++20 /EHsc /MTd /O2 /D_DEBUG=1 '+$taskDefines+' '+($AdditionalCompilerOptions -join ' ')+' /I"'+(Join-Path $taskRoot 'src')+'" /I"'+$taskRoot+'" /I"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include" /c '+$taskSources+' /Fo"'+$taskArchOutput+'/"'
    $taskCommand|Out-File (Join-Path $taskArchOutput 'command.txt') -Encoding utf8
    $ErrorActionPreference='Continue'
    & cmd /d /s /c $taskCommand *> (Join-Path $taskArchOutput 'compile.log')
    $taskExit=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    foreach($taskUnit in $taskHashes.Keys){if((Get-FileHash -LiteralPath (Join-Path $taskRoot $taskUnit) -Algorithm SHA256).Hash -ne $taskHashes[$taskUnit]){throw 'Source changed during normal build'}}
    @{architecture=$taskArch;exit=$taskExit;completed=($taskExit -eq 0);native_selection=$false;translation_units=$TranslationUnits.Count}|ConvertTo-Json|Out-File (Join-Path $taskArchOutput 'build-result.json') -Encoding utf8
    if($taskExit){Get-Content (Join-Path $taskArchOutput 'compile.log') -Tail 20;throw 'Normal Windows startup closure build failed'}
}
