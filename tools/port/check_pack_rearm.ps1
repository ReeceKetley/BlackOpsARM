param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat')
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath (Resolve-Path "$PSScriptRoot\..\..").Path
python "$PSScriptRoot\check_pack_rearm.py"
if ($LASTEXITCODE -ne 0) { throw 'Pack recovery test generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_pack_rearm_$arch.exe"
    $log = "build\run\test_pack_rearm_$arch.log"
    & cmd /d /s /c "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_pack_rearm.cpp /Fe$exe /Fobuild\run\test_pack_rearm_$arch.obj" > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch pack recovery test compilation failed" }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    if ($LASTEXITCODE -ne 0) { throw "$arch pack recovery test failed" }
}
python "$PSScriptRoot\check_pack_rearm.py" --baseline
if ($LASTEXITCODE -ne 0) { throw 'Negative control generation failed' }
$exe = 'build\run\test_pack_rearm_baseline.exe'
$log = 'build\run\test_pack_rearm_baseline.log'
& cmd /d /s /c "`"$VcVars`" x64 >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_pack_rearm_baseline.cpp /Fe$exe /Fobuild\run\test_pack_rearm_baseline.obj" > $log 2>&1
if ($LASTEXITCODE -ne 0) { Get-Content $log; throw 'Negative control compilation failed' }
& ".\$exe" | Tee-Object -FilePath $log -Append
$baselineExit = $LASTEXITCODE
"Unfixed PackCheck negative-control exit: $baselineExit" | Tee-Object -FilePath $log -Append
if ($baselineExit -eq 0) { throw 'Unfixed portal readiness unexpectedly passed' }
