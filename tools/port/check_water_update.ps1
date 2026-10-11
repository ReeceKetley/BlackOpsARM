param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat',
      [string]$Fixture = 'build\run\water_update50.bin',
      [ValidateSet('precise','strict')][string]$FloatingPoint = 'precise',
      [switch]$NegativeControl, [switch]$NegativeConfig)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeConfig) { '_config_baseline' } elseif ($NegativeControl) { '_baseline' } else { '' }
if ($NegativeConfig) { python "$PSScriptRoot\check_water_update.py" --config-baseline }
elseif ($NegativeControl) { python "$PSScriptRoot\check_water_update.py" --baseline }
else { python "$PSScriptRoot\check_water_update.py" }
if ($LASTEXITCODE -ne 0) { throw 'Update test generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_water_update${suffix}_$arch.exe"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 /fp:$FloatingPoint build\run\test_water_update$suffix.cpp /Fe$exe /Fobuild\run\test_water_update${suffix}_$arch.obj /link /MANIFEST:EMBED"
    $log = "build\run\test_water_update${suffix}_$arch.log"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch update compilation failed" }
    & ".\$exe" $Fixture | Tee-Object -FilePath $log -Append
    $result = $LASTEXITCODE
    if ($NegativeControl -or $NegativeConfig) { if ($result -eq 0) { throw 'Unfixed update unexpectedly passed' } }
    elseif ($result -ne 0) { throw "$arch update failed" }
}
