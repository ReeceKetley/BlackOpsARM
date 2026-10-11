param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat', [switch]$NegativeControl)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeControl) { '_baseline' } else { '' }
if ($NegativeControl) { python "$PSScriptRoot\check_water_scheduler.py" --baseline }
else { python "$PSScriptRoot\check_water_scheduler.py" }
if ($LASTEXITCODE -ne 0) { throw 'Scheduler test generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_water_scheduler${suffix}_$arch.exe"
    $log = "build\run\test_water_scheduler${suffix}_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_water_scheduler$suffix.cpp /Fe$exe /Fobuild\run\test_water_scheduler${suffix}_$arch.obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch scheduler compilation failed" }
    & ".\$exe" build\run\water_scheduler53.bin | Tee-Object -FilePath $log -Append
    $result = $LASTEXITCODE
    if ($NegativeControl) { if ($result -eq 0) { throw 'Old scheduler unexpectedly passed' } }
    elseif ($result -ne 0) { throw "$arch scheduler failed" }
}
