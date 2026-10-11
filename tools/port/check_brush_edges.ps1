param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
python "$PSScriptRoot\check_brush_edges.py"
if ($LASTEXITCODE -ne 0) { throw 'Brush test generation failed' }
foreach ($arch in 'x86','x64') {
    $defines = if ($arch -eq 'x64') { '/DBO1_PORT_X64' } else { '' }
    $exe = "build\run\test_brush_edges_$arch.exe"
    $obj = "build\run\test_brush_edges_$arch.obj"
    $log = "build\run\test_brush_edges_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /w /std:c++20 /EHsc /O2 $defines /Isrc build\run\test_brush_edges.cpp /Fe$exe /Fo$obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch brush test compilation failed" }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    if ($LASTEXITCODE -ne 0) { throw "$arch brush test failed" }
}
