param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat',
      [string]$Fixture = 'build\run\water_retail49.bin', [switch]$Before, [switch]$BuildGpu, [switch]$NegativeControl)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeControl) { '_baseline' } elseif ($Before) { '_before' } else { '' }
if ($NegativeControl) { python "$PSScriptRoot\check_water_skin.py" --baseline }
elseif ($Before) { python "$PSScriptRoot\check_water_skin.py" --before }
else { python "$PSScriptRoot\check_water_skin.py" }
if ($LASTEXITCODE -ne 0) { throw 'Skin fixture generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_water_skin${suffix}_$arch.exe"
    $obj = "build\run\test_water_skin${suffix}_$arch.obj"
    $log = "build\run\test_water_skin${suffix}_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 /fp:strict build\run\test_water_skin$suffix.cpp /Fe$exe /Fo$obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch skin compilation failed" }
    & ".\$exe" $Fixture | Tee-Object -FilePath $log -Append
    $result = $LASTEXITCODE
    "Exit: $result" | Tee-Object -FilePath $log -Append
    if ($Before -or $NegativeControl) { if ($result -eq 0) { throw 'Old skin unexpectedly matched retail' } }
    elseif ($result -ne 0) { throw "$arch skin failed" }
}
if ($BuildGpu) {
    $exe = 'build\run\test_water_skin_gpu_x64.exe'
    $command = "`"$VcVars`" x64 >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 /fp:strict /DWATER_GPU build\run\test_water_skin.cpp /Fe$exe /Fobuild\run\test_water_skin_gpu_x64.obj /link d3d9.lib user32.lib"
    & cmd /d /s /c $command > build\run\test_water_skin_gpu_compile.log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content build\run\test_water_skin_gpu_compile.log; throw 'GPU fixture compile failed' }
    Write-Output "Built $exe; stage a pinned DXVK DLL beside it before running"
}
