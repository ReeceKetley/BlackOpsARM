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
    $native=if($arch -eq 'x64'){' /DBO1_PORT_X64=1'}else{''}
    $files=@('tools/port/tests/string_pool_tests.cpp','tools/port/tests/string_pool_support.cpp','src/clientscript/cscr_stringlist.cpp','src/clientscript/cscr_memorytree.cpp','src/clientscript/cscr_instance.cpp','src/platform/android/script_pool_bridge.cpp','src/platform/android/script_string_services.cpp')
    $sources=($files|ForEach-Object{'"'+(Join-Path $probeRoot $_)+'"'}) -join ' '
    $command='"'+$VcVars+'" '+$arch+' >nul && cl /nologo /W4 /std:c++20 /EHsc /MTd /O2 /Gy /D_DEBUG=1'+
        $native+' /DBO1_SCRIPT_STRING_SERVICES_HOST_TEST=1 /DBO1_STRING_POOL_GATE=1 /I"'+(Join-Path $probeRoot 'src')+'" /I"'+$probeRoot+'" '+$sources+
        ' /Fe"'+(Join-Path $output 'string_pool.exe')+'" /Fo"'+$output+'/" /link /OPT:REF /MAP'
    $command|Out-File (Join-Path $output 'command.txt') -Encoding utf8
    & cmd /d /s /c $command 2>&1|Out-File (Join-Path $output 'compile.log') -Encoding utf8
    if($LASTEXITCODE){Get-Content (Join-Path $output 'compile.log') -Tail 25;throw "$arch compile failed"}
}
Write-Output "Actual Windows SL/MT pool: $probeOutput"
