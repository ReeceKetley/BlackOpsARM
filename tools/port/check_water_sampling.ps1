param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat', [switch]$NegativeControl)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeControl) { '_baseline' } else { '' }
if ($NegativeControl) { python "$PSScriptRoot\check_water_sampling.py" --baseline }
else { python "$PSScriptRoot\check_water_sampling.py" }
if ($LASTEXITCODE -ne 0) { throw 'Sampling fixture generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_water_sampling${suffix}_$arch.exe"
    $log = "build\run\test_water_sampling${suffix}_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 /fp:precise build\run\test_water_sampling$suffix.cpp /Fe$exe /Fobuild\run\test_water_sampling${suffix}_$arch.obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch sampling compilation failed" }
    & ".\$exe" build\run\water_sampling55.bin 2>&1 | Tee-Object -FilePath $log -Append
    $result = $LASTEXITCODE
    if ($NegativeControl) { if ($result -eq 0) { throw 'Old sampling unexpectedly passed' } }
    elseif ($result -ne 0) { throw "$arch sampling failed" }
}
