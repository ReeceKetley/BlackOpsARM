param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat', [switch]$NegativeControl)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
$suffix = if ($NegativeControl) { '_baseline' } else { '' }
if ($NegativeControl) { python "$PSScriptRoot\check_ammo_target.py" --baseline }
else { python "$PSScriptRoot\check_ammo_target.py" }
if ($LASTEXITCODE -ne 0) { throw 'Ammo target generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_ammo_target${suffix}_$arch.exe"
    $log = "build\run\test_ammo_target${suffix}_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_ammo_target$suffix.cpp /Fe$exe /Fobuild\run\test_ammo_target${suffix}_$arch.obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch ammo target compile failed" }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    $result = $LASTEXITCODE
    if ($NegativeControl) { if ($result -eq 0) { throw 'Old ammo target unexpectedly passed' } }
    elseif ($result -ne 0) { throw "$arch ammo target failed" }
}
