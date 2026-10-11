param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat', [switch]$NegativeControl, [switch]$NegativeDistance)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeDistance) { '_distance_baseline' } elseif ($NegativeControl) { '_baseline' } else { '' }
if ($NegativeDistance) { python "$PSScriptRoot\check_water_track.py" --distance-baseline }
elseif ($NegativeControl) { python "$PSScriptRoot\check_water_track.py" --baseline }
else { python "$PSScriptRoot\check_water_track.py" }
if ($LASTEXITCODE -ne 0) { throw 'Tracking fixture generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_water_track${suffix}_$arch.exe"
    $log = "build\run\test_water_track${suffix}_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 /fp:precise build\run\test_water_track$suffix.cpp /Fe$exe /Fobuild\run\test_water_track${suffix}_$arch.obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch tracking compilation failed" }
    & ".\$exe" build\run\water_track54.bin 2>&1 | Tee-Object -FilePath $log -Append
    $result = $LASTEXITCODE
    if ($NegativeControl -or $NegativeDistance) { if ($result -eq 0) { throw 'Old tracking unexpectedly passed' } }
    elseif ($result -ne 0) { throw "$arch tracking failed" }
}
