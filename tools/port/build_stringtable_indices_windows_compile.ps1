param([string]$VcVars='C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat', [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$probeRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$probeOutput=[IO.Path]::GetFullPath((Join-Path $probeRoot $OutputDirectory))
if (!$probeOutput.StartsWith($probeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Only Android worktree outputs allowed' }
if(Test-Path -LiteralPath $probeOutput){throw 'Use fresh output directory'}
New-Item -ItemType Directory -Path $probeOutput | Out-Null
foreach($arch in @('x86','x64')){
    $output=Join-Path $probeOutput $arch
    New-Item -ItemType Directory -Path $output | Out-Null
    $defines=if($arch -eq 'x64'){'/DBO1_PORT_X64=1'}else{''}
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1 '+$defines+
        ' /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" /I"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include"'+
        ' /c "'+(Join-Path $probeRoot 'src/database/db_load.cpp')+'" /Fo"'+$output+'/"'
    $command|Out-File (Join-Path $output 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1|Out-File (Join-Path $output 'compile.log') -Encoding utf8
    if($LASTEXITCODE){Get-Content (Join-Path $output 'compile.log') -Tail 25;throw "$arch compile failed"}
}
Write-Output "Normal Windows full db_load.cpp compile evidence (INDEX gate disabled): $probeOutput"
