param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat', [switch]$NegativeControl, [switch]$NegativeHalo)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeHalo) { '_halo_baseline' } elseif ($NegativeControl) { '_baseline' } else { '' }
if ($NegativeHalo) { python "$PSScriptRoot\check_water_cache.py" --halo-baseline }
elseif ($NegativeControl) { python "$PSScriptRoot\check_water_cache.py" --baseline }
else { python "$PSScriptRoot\check_water_cache.py" }
if ($LASTEXITCODE -ne 0) { throw 'Cache test generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_water_cache${suffix}_$arch.exe"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_water_cache$suffix.cpp /Fe$exe /Fobuild\run\test_water_cache${suffix}_$arch.obj"
    $log = "build\run\test_water_cache${suffix}_$arch.log"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch cache compilation failed" }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    $result = $LASTEXITCODE
    if ($NegativeControl -or $NegativeHalo) { if ($result -eq 0) { throw 'Unfixed cache unexpectedly passed' } }
    elseif ($result -ne 0) { throw "$arch cache failed" }
}
