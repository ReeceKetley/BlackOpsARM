param([string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat',
    [string]$OutputDirectory=('build-android-audit/assetlist_headers-'+[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff')))
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput=[IO.Path]::GetFullPath((Join-Path $probeRoot $OutputDirectory))
if (!$probeOutput.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Only Android worktree outputs allowed' }
if(Test-Path -LiteralPath $probeOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach($selection in @('x86','x64','x64-flag-off')){
    $arch=if($selection -eq 'x86'){'x86'}else{'x64'}
    $defines=if($selection -eq 'x64'){'/DBO1_PORT_X64=1'}elseif($selection -eq 'x64-flag-off'){'/DBO1_ASSETLIST_LIGHT_LAYOUT=1'}else{''}
    $output=Join-Path $probeOutput $selection
    New-Item -ItemType Directory -Path $output | Out-Null
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /D_DEBUG=1 '+$defines+
        ' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" "'+(Join-Path $probeRoot 'tools/port/tests/assetlist_layout_probe.cpp')+'" "'+(Join-Path $probeRoot 'tools/port/tests/checked_zone_support.cpp')+'" /Fe"'+(Join-Path $output 'layout.exe')+'" /Fo"'+$output+'/"'
    $command|Out-File (Join-Path $output 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1|Out-File (Join-Path $output 'compile.log') -Encoding utf8
    if($LASTEXITCODE){throw "$selection full-registry compile failed"}
    & (Join-Path $output 'layout.exe')|Tee-Object (Join-Path $output 'test.log')
    if($LASTEXITCODE){throw "$selection layout failed"}
}
# Normal full Windows x64 expander TU: no bounded gate macro. This is compile-only;
# neither the Windows checkout nor engine/backend is built or launched.
$command='"'+$VcVars+'" x64 >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /D_DEBUG=1 /DBO1_PORT_X64=1 /c'+
    ' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" /I"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include"'+
    ' "'+(Join-Path $probeRoot 'src/database/db_expand.cpp')+'" /Fo"'+(Join-Path $probeOutput 'full-windows-db-expand.obj')+'"'
$command|Out-File (Join-Path $probeOutput 'full-command.txt') -Encoding utf8
& cmd /d /s /c $command 2>&1|Out-File (Join-Path $probeOutput 'full-compile.log') -Encoding utf8
if($LASTEXITCODE){Get-Content (Join-Path $probeOutput 'full-compile.log') -Tail 20;throw 'Normal Windows expander compile failed'}
Write-Output "PASS full normal Windows expander compile: $probeOutput"
