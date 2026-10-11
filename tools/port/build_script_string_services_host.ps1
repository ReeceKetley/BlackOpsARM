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
    $files=@('tools/port/tests/script_string_services_tests.cpp','src/platform/android/script_string_services.cpp')
    $sources=($files|ForEach-Object{'"'+(Join-Path $probeRoot $_)+'"'}) -join ' '
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1'+
        ' /DBO1_SCRIPT_STRING_SERVICES_HOST_TEST=1 /I"'+(Join-Path $probeRoot 'src')+'" '+$sources+
        ' /Fe"'+(Join-Path $output 'script_string_services.exe')+'" /Fo"'+$output+'/" /link /OPT:REF /MAP'
    $command|Out-File (Join-Path $output 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1|Out-File (Join-Path $output 'compile.log') -Encoding utf8
    if($LASTEXITCODE){Get-Content (Join-Path $output 'compile.log') -Tail 25;throw "$arch compile failed"}
}
Write-Output "Real Windows script-pool platform services: $probeOutput"
