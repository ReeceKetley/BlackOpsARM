param([string]$VcVars = 'C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat', [switch]$NegativeControl)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location -LiteralPath $root
python "$PSScriptRoot\check_glass_allocator.py"
if ($LASTEXITCODE -ne 0) { throw 'Glass allocator test generation failed' }
foreach ($arch in 'x86','x64') {
    $exe = "build\run\test_glass_allocator_$arch.exe"
    $obj = "build\run\test_glass_allocator_$arch.obj"
    $log = "build\run\test_glass_allocator_$arch.log"
    $command = "`"$VcVars`" $arch >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_glass_allocator.cpp /Fe$exe /Fo$obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw "$arch glass test compilation failed" }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    if ($LASTEXITCODE -ne 0) { throw "$arch glass test failed" }
}
if ($NegativeControl) {
    python "$PSScriptRoot\check_glass_allocator.py" --baseline
    if ($LASTEXITCODE -ne 0) { throw 'Negative control generation failed' }
    $exe = 'build\run\test_glass_allocator_baseline_x64.exe'
    $obj = 'build\run\test_glass_allocator_baseline_x64.obj'
    $log = 'build\run\test_glass_allocator_baseline_x64.log'
    $command = "`"$VcVars`" x64 >nul && cl /nologo /W3 /std:c++20 /EHsc /O2 build\run\test_glass_allocator_baseline.cpp /Fe$exe /Fo$obj"
    & cmd /d /s /c $command > $log 2>&1
    if ($LASTEXITCODE -ne 0) { Get-Content $log; throw 'Negative control compilation failed' }
    & ".\$exe" | Tee-Object -FilePath $log -Append
    $baselineExit = $LASTEXITCODE
    "Unfixed x64 allocator negative-control exit: $baselineExit" | Tee-Object -FilePath $log -Append
    if ($baselineExit -eq 0) { throw 'Unfixed allocator unexpectedly passed' }
}
