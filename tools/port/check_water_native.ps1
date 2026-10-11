param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat', [switch]$NegativeControl)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
python "$PSScriptRoot\check_water_native.py"
if ($LASTEXITCODE -ne 0) { throw 'Water test generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_water_native_$arch.exe"
    $obj = "build\run\test_water_native_$arch.obj"
    $log = "build\run\test_water_native_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_water_native.cpp /Fe$exe /Fo$obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch water test compilation failed" }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    if ($LASTEXITCODE -ne 0) { throw "$arch water test failed" }
}
if ($NegativeControl) {
    python "$PSScriptRoot\check_water_native.py" --baseline
    if ($LASTEXITCODE -ne 0) { throw 'Negative control generation failed' }
    $exe = 'build\run\test_water_native_baseline_x64.exe'
    $obj = 'build\run\test_water_native_baseline_x64.obj'
    $log = 'build\run\test_water_native_baseline_x64.log'
    $command = "`"$VcVars`" x64 >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_water_native_baseline.cpp /Fe$exe /Fo$obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw 'Negative control compilation failed' }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    $baselineExit = $LASTEXITCODE
    "Unfixed x64 water negative-control exit: $baselineExit" | Tee-Object -FilePath $log -Append
    if ($baselineExit -eq 0) { throw 'Unfixed water surfaces unexpectedly passed' }
    & ".\$exe" --surfaces | Tee-Object -FilePath $log -Append
    $surfaceExit = $LASTEXITCODE
    "Unfixed x64 surface negative-control exit: $surfaceExit" | Tee-Object -FilePath $log -Append
    if ($surfaceExit -eq 0) { throw 'Unfixed surface packing unexpectedly passed' }
    $exe32 = 'build\run\test_water_native_baseline_x86.exe'
    $obj32 = 'build\run\test_water_native_baseline_x86.obj'
    $log32 = 'build\run\test_water_native_baseline_x86.log'
    $command = "`"$VcVars`" x86 >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_water_native_baseline.cpp /Fe$exe32 /Fo$obj32"
    & cmd /d /s /c $command > $log32 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log32; throw 'Win32 negative control compilation failed' }
    & ".\$exe32" --lookup | Tee-Object -FilePath $log32 -Append
    $lookupExit = $LASTEXITCODE
    "Unfixed Win32 channel-lookup negative-control exit: $lookupExit" | Tee-Object -FilePath $log32 -Append
    if ($lookupExit -eq 0) { throw 'Unfixed channel lookup unexpectedly passed' }
}
